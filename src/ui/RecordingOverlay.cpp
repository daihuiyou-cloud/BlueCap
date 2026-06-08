#include "RecordingOverlay.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>

#include <windows.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x11
#endif

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
    m_isFullscreen = false;
    setGeometry(region.adjusted(-4, -4, 4, 4));
    m_pulseState = false;
    m_pulseTimer->start();
    show();
    raise();
}

void RecordingOverlay::showForFullscreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    m_area = screen->geometry();
    m_isFullscreen = true;
    setGeometry(m_area.adjusted(-4, -4, 4, 4));
    m_pulseState = false;
    m_pulseTimer->start();
    show();
    raise();
}

void RecordingOverlay::hideOverlay()
{
    m_pulseTimer->stop();
    hide();
}

void RecordingOverlay::showEvent(QShowEvent *event)
{
    HWND hwnd = (HWND)winId();
    SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    QWidget::showEvent(event);
}

void RecordingOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF inner(4, 4, width() - 8, height() - 8);
    const int radius = 8;

    for (int i = 3; i >= 1; --i) {
        QColor glow(9, 103, 242, 10 + i * 8);
        QPen pen(glow, i + 1);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(inner.adjusted(-i, -i, i, i), radius + i, radius + i);
    }

    QPen mainPen(QColor("#0967f2"), 2);
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
}
