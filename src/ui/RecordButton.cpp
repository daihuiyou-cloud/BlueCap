#include "RecordButton.h"

#include <QPainter>
#include <QPainterPath>

RecordButton::RecordButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setToolTip(QStringLiteral("开始/停止录制"));
}

QSize RecordButton::sizeHint() const
{
    return QSize(190, 190);
}

void RecordButton::setRecording(bool recording)
{
    if (m_recording == recording) {
        return;
    }

    m_recording = recording;
    update();
}

void RecordButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(8, 8, -8, -8);
    const QPointF center = bounds.center();
    const qreal radius = qMin(bounds.width(), bounds.height()) / 2.0;

    QRadialGradient shadow(center + QPointF(0, 16), radius * 1.25);
    shadow.setColorAt(0.2, QColor(36, 100, 220, 70));
    shadow.setColorAt(1.0, QColor(36, 100, 220, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadow);
    painter.drawEllipse(center, radius * 1.08, radius * 1.08);

    painter.setBrush(QColor(246, 250, 255));
    painter.drawEllipse(bounds);

    QLinearGradient blue(bounds.topLeft(), bounds.bottomRight());
    blue.setColorAt(0.0, m_recording ? QColor(255, 89, 89) : QColor(77, 166, 255));
    blue.setColorAt(1.0, m_recording ? QColor(210, 29, 45) : QColor(10, 91, 238));
    painter.setBrush(blue);
    painter.drawEllipse(bounds.adjusted(22, 22, -22, -22));

    painter.setBrush(QColor(255, 255, 255));
    painter.drawEllipse(bounds.adjusted(68, 68, -68, -68));

    painter.setBrush(m_recording ? QColor(255, 255, 255) : QColor(239, 48, 57));
    if (m_recording) {
        const QRectF stopRect(center.x() - 19, center.y() - 19, 38, 38);
        painter.drawRoundedRect(stopRect, 8, 8);
    } else {
        painter.drawEllipse(center, 30, 30);
    }
}
