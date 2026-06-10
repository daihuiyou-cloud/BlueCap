#include "RecordingOverlay.h"
#include "theme/ThemeColors.h"

#include <QFontMetrics>
#include <QOperatingSystemVersion>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>

#include <windows.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x11
#endif

namespace {

bool supportsExcludeFromCapture()
{
    return QOperatingSystemVersion::current() >=
        QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 19041);
}

}

RecordingOverlay::RecordingOverlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(800);
    connect(m_pulseTimer, &QTimer::timeout, this, [this] {
        m_pulseState = !m_pulseState;
        update();
    });
}

void RecordingOverlay::showForRegion(const QRect &region)
{
    m_area = region;
    setGeometry(region.adjusted(-4, -4, 4, 4));
    m_pulseState = false;
    m_pulseTimer->start();
    show();
    raise();
}

void RecordingOverlay::hideOverlay()
{
    m_pulseTimer->stop();
    m_statusText.clear();
    m_hintText.clear();
    hide();
}

void RecordingOverlay::setStatusText(const QString &text)
{
    m_statusText = text;
    update();
}

void RecordingOverlay::setHintText(const QString &text)
{
    m_hintText = text;
    update();
}

void RecordingOverlay::showEvent(QShowEvent *event)
{
    if (supportsExcludeFromCapture()) {
        HWND hwnd = (HWND)winId();
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    }
    QWidget::showEvent(event);
}

void RecordingOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(false).app;
    const QColor overlayColor = a.inputFocusBorder;

    const QRectF inner(4, 4, width() - 8, height() - 8);
    const int radius = 8;

    for (int i = 3; i >= 1; --i) {
        int alpha = 10 + (4 - i) * 8;
        int penWidth = i + 1;
        qreal hw = penWidth / 2.0;
        QPen pen(QColor(overlayColor.red(), overlayColor.green(), overlayColor.blue(), alpha), penWidth);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(inner.adjusted(-i + hw, -i + hw, i - hw, i - hw), radius + i - hw, radius + i - hw);
    }

    QPen mainPen(overlayColor, 2);
    painter.setPen(mainPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(inner, radius, radius);

    {
        const qreal dotR = 4;
        QPointF dotCenter(inner.right() - 14, inner.top() + 14);
        int alpha = m_pulseState ? 230 : 100;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(239, 48, 57, alpha));
        painter.drawEllipse(dotCenter, dotR, dotR);
    }

    if (!m_statusText.isEmpty()) {
        const int bx = 16;
        const int by = 16;
        const int padX = 16;
        const int padY = 7;
        const int padYb = 7;
        const int gap = 4;

        QFont font = painter.font();
        font.setPointSize(11);
        font.setBold(true);
        QFontMetrics fm(font);
        QString text = QStringLiteral("●  %1").arg(m_statusText);
        int textW = fm.horizontalAdvance(text);

        int hintW = 0;
        int hintH = 0;
        QFont hintFont;
        QFontMetrics hfm(font);
        if (!m_hintText.isEmpty()) {
            hintFont = font;
            hintFont.setPointSize(10);
            hintFont.setBold(false);
            hfm = QFontMetrics(hintFont);
            hintW = hfm.horizontalAdvance(m_hintText);
            hintH = hfm.height();
        }

        int cw = qMax(textW, hintW);
        int ch = fm.height() + (m_hintText.isEmpty() ? 0 : gap + hintH);
        int bw = cw + padX * 2;
        int bh = ch + padY + padYb;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.drawRoundedRect(bx, by, bw, bh, 8, 8);

        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(bx + padX, by + padY + fm.ascent(), text);

        if (!m_hintText.isEmpty()) {
            painter.setFont(hintFont);
            painter.setPen(QColor(200, 200, 200));
            painter.drawText(bx + padX, by + padY + fm.height() + gap + hfm.ascent(), m_hintText);
        }
    }
}
