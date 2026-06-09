#include "AudioToggleCard.h"

#include "paint/PaintMetrics.h"
#include "paint/PaintPrimitives.h"

#include <QMouseEvent>
#include <QPainter>

AudioToggleCard::AudioToggleCard(const QString &title, Accent accent, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_enabledText(QStringLiteral("已开启"))
    , m_disabledText(QStringLiteral("已关闭"))
    , m_accent(accent)
{
    setFixedSize(paint::Metrics::audioCardWidth, paint::Metrics::audioCardHeight);
    setCursor(Qt::PointingHandCursor);
    m_palette = paint::theme(false);
}

void AudioToggleCard::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_palette = paint::theme(dark);
    update();
}

void AudioToggleCard::setAudioEnabled(bool on)
{
    if (m_audioEnabled == on) return;
    m_audioEnabled = on;
    update();
    emit toggled(on);
}

void AudioToggleCard::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange)
        update();
    QWidget::changeEvent(event);
}

void AudioToggleCard::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void AudioToggleCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void AudioToggleCard::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled()) { QWidget::mousePressEvent(event); return; }
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
    QWidget::mousePressEvent(event);
}

void AudioToggleCard::mouseReleaseEvent(QMouseEvent *event)
{
    if (!isEnabled()) { QWidget::mouseReleaseEvent(event); return; }
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        if (rect().contains(event->pos()))
            setAudioEnabled(!m_audioEnabled);
        else
            update();
    }
    QWidget::mouseReleaseEvent(event);
}

void AudioToggleCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (!isEnabled())
        p.setOpacity(0.45);

    const auto &pal = m_palette;
    QRectF r = QRectF(rect());

    QColor bg = m_pressed ? pal.cardCheckedBg
               : m_hovered ? pal.cardHoverBg
                           : pal.cardBg;
    paint::drawCard(p, r, bg, pal.cardBorder, 12);
    paint::drawVerticalSheen(p, r, 12, m_darkMode);

    const QColor accent = m_accent == Microphone ? pal.primary : pal.accentGreen;
    const QColor iconColor = m_audioEnabled ? accent : pal.faintText;
    const QColor waveColor = QColor(iconColor.red(), iconColor.green(), iconColor.blue(), 120);
    QRectF iconRect(17, 19, 24, 24);

    p.setPen(QPen(iconColor, 2));
    p.setBrush(Qt::NoBrush);
    if (m_accent == Microphone) {
        p.drawRoundedRect(QRectF(iconRect.center().x() - 5, iconRect.top() + 2, 10, 15), 5, 5);
        p.drawLine(iconRect.center().x(), iconRect.bottom() - 5, iconRect.center().x(), iconRect.bottom());
        p.drawLine(iconRect.center().x() - 7, iconRect.bottom(), iconRect.center().x() + 7, iconRect.bottom());
    } else {
        p.drawLine(iconRect.left() + 2, iconRect.center().y(), iconRect.left() + 8, iconRect.center().y());
        p.drawLine(iconRect.left() + 8, iconRect.center().y(), iconRect.left() + 15, iconRect.top() + 6);
        p.drawLine(iconRect.left() + 15, iconRect.top() + 6, iconRect.left() + 15, iconRect.bottom() - 6);
        p.drawLine(iconRect.left() + 15, iconRect.bottom() - 6, iconRect.left() + 8, iconRect.center().y());
        p.drawArc(QRectF(iconRect.left() + 14, iconRect.top() + 5, 12, 14), -45 * 16, 90 * 16);
    }

    QFont titleFont = p.font();
    titleFont.setPixelSize(13);
    titleFont.setBold(true);
    p.setFont(titleFont);
    paint::drawElidedText(p, QRect(49, 17, width() - 60, 20),
                          Qt::AlignLeft | Qt::AlignVCenter, m_title, pal.text);

    QFont stateFont = p.font();
    stateFont.setPixelSize(12);
    stateFont.setBold(false);
    p.setFont(stateFont);
    const QString &stateText = m_audioEnabled ? m_enabledText : m_disabledText;
    const QColor stateColor = m_audioEnabled ? accent : pal.faintText;
    paint::drawElidedText(p, QRect(49, 38, width() - 60, 18),
                          Qt::AlignLeft | Qt::AlignVCenter, stateText, stateColor);

    p.setPen(QPen(waveColor, 1));
    const int baseY = height() - 17;
    for (int i = 0; i < 18; ++i) {
        int h = m_audioEnabled ? (3 + ((i * 7) % 11)) : 2;
        int x = 37 + i * 4;
        p.drawLine(x, baseY - h / 2, x, baseY + h / 2);
    }
}
