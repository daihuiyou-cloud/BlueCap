#include "RecordingIndicator.h"

#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QTimer>

RecordingIndicator::RecordingIndicator(QWidget *parent)
    : QLabel(parent)
{
    setVisible(false);
    setFixedHeight(28);
    m_palette = paint::theme(false);
    rebuildPath();
}

void RecordingIndicator::setDarkMode(bool dark)
{
    m_darkMode = dark;
    m_palette = paint::theme(dark);
    update();
}

void RecordingIndicator::startPulse()
{
    if (!m_pulseTimer) {
        m_pulseTimer = new QTimer(this);
        connect(m_pulseTimer, &QTimer::timeout, this, [this] {
            m_pulseState = !m_pulseState;
            update();
        });
    }
    m_pulseState = true;
    update();
    m_pulseTimer->start(800);
}

void RecordingIndicator::stopPulse()
{
    if (m_pulseTimer)
        m_pulseTimer->stop();
    update();
}

void RecordingIndicator::rebuildPath()
{
    m_cachedPath = QPainterPath();
    m_cachedPath.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
}

void RecordingIndicator::resizeEvent(QResizeEvent *event)
{
    rebuildPath();
    QLabel::resizeEvent(event);
}

void RecordingIndicator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QColor baseRed = m_palette.danger;

    int alpha = m_pulseState ? 200 : 60;
    int borderAlpha = m_pulseState ? 250 : 80;

    QColor bg(baseRed.red(), baseRed.green(), baseRed.blue(),
              m_darkMode ? qMin(alpha + 60, 220) : qMin(alpha, 120));
    QColor border(baseRed.red(), baseRed.green(), baseRed.blue(),
                  m_darkMode ? qMin(borderAlpha + 60, 250) : qMin(borderAlpha, 150));

    painter.setPen(QPen(border, 1));
    painter.setBrush(bg);
    painter.drawPath(m_cachedPath);

    QFont font = painter.font();
    font.setPixelSize(13);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(baseRed);
    painter.drawText(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                     Qt::AlignCenter, QStringLiteral("● 录制中"));
}
