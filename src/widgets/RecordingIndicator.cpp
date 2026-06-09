#include "RecordingIndicator.h"

#include <QPainter>
#include <QPainterPath>
#include <QTimer>

RecordingIndicator::RecordingIndicator(QWidget *parent)
    : QLabel(parent)
{
    setVisible(false);
    setFixedHeight(28);
    m_palette = paint::theme(false);
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
    m_pulseState = false;
    m_pulseTimer->start(800);
}

void RecordingIndicator::stopPulse()
{
    if (m_pulseTimer)
        m_pulseTimer->stop();
    update();
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

    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 12, 12);

    painter.setPen(QPen(border, 1));
    painter.setBrush(bg);
    painter.drawPath(path);

    QFont font = painter.font();
    font.setPixelSize(13);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(baseRed);
    painter.drawText(r, Qt::AlignCenter, QStringLiteral("● 录制中"));
}
