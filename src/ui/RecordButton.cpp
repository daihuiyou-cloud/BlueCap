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
    return QSize(110, 110);
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

    const QRectF bounds = rect().adjusted(4, 4, -4, -4);
    const QPointF center = bounds.center();
    const qreal d = qMin(bounds.width(), bounds.height());
    const qreal r = d / 2.0;

    QRadialGradient shadow(center + QPointF(0, d * 0.1), r * 1.25);
    shadow.setColorAt(0.2, QColor(36, 100, 220, 60));
    shadow.setColorAt(1.0, QColor(36, 100, 220, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadow);
    painter.drawEllipse(center, r * 1.08, r * 1.08);

    painter.setBrush(QColor(246, 250, 255));
    painter.drawEllipse(bounds);

    QLinearGradient blue(bounds.topLeft(), bounds.bottomRight());
    blue.setColorAt(0.0, m_recording ? QColor(255, 89, 89) : QColor(77, 166, 255));
    blue.setColorAt(1.0, m_recording ? QColor(210, 29, 45) : QColor(10, 91, 238));

    const qreal g = d * 0.126;
    painter.setBrush(blue);
    painter.drawEllipse(bounds.adjusted(g, g, -g, -g));

    const qreal w = d * 0.391;
    painter.setBrush(QColor(255, 255, 255));
    painter.drawEllipse(bounds.adjusted(w, w, -w, -w));

    painter.setBrush(m_recording ? QColor(255, 255, 255) : QColor(239, 48, 57));
    if (m_recording) {
        const qreal s = d * 0.109;
        painter.drawRoundedRect(QRectF(center.x() - s, center.y() - s, s * 2, s * 2), 4, 4);
    } else {
        painter.drawEllipse(center, d * 0.172, d * 0.172);
    }
}
