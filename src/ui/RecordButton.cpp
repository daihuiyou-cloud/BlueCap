#include "RecordButton.h"

#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

RecordButton::RecordButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setToolTip(QStringLiteral("开始/停止录制"));
    setAccessibleName(QStringLiteral("录制按钮"));
}

QSize RecordButton::sizeHint() const
{
    return QSize(130, 130);
}

void RecordButton::setRecording(bool recording)
{
    if (m_recording == recording)
        return;

    m_recording = recording;
    update();
}

void RecordButton::resizeEvent(QResizeEvent *event)
{
    renderCache();
    QAbstractButton::resizeEvent(event);
}

void RecordButton::setDarkMode(bool dark)
{
    if (m_darkMode == dark) return;
    m_darkMode = dark;
    renderCache();
    update();
}

void RecordButton::renderCache()
{
    m_bgCache = QPixmap(size());
    m_bgCache.fill(Qt::transparent);

    QPainter p(&m_bgCache);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(4, 4, -4, -4);
    const QPointF center = bounds.center();
    const qreal d = qMin(bounds.width(), bounds.height());
    const qreal r = d / 2.0;

    QRadialGradient shadow(center + QPointF(0, d * 0.1), r * 1.25);
    int shadowAlpha = m_darkMode ? 30 : 60;
    shadow.setColorAt(0.2, QColor(36, 100, 220, shadowAlpha));
    shadow.setColorAt(1.0, QColor(36, 100, 220, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(shadow);
    p.drawEllipse(center, r * 1.08, r * 1.08);

    p.setBrush(m_darkMode ? QColor(45, 52, 66) : QColor(246, 250, 255));
    p.drawEllipse(bounds);

    const qreal w = d * 0.391;
    p.setBrush(m_darkMode ? QColor(35, 42, 55) : QColor(255, 255, 255));
    p.drawEllipse(bounds.adjusted(w, w, -w, -w));
}

void RecordButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_bgCache.isNull())
        painter.drawPixmap(0, 0, m_bgCache);

    const QRectF bounds = rect().adjusted(4, 4, -4, -4);
    const QPointF center = bounds.center();
    const qreal d = qMin(bounds.width(), bounds.height());

    QLinearGradient blue(bounds.topLeft(), bounds.bottomRight());
    blue.setColorAt(0.0, m_recording ? QColor(255, 89, 89) : QColor(77, 166, 255));
    blue.setColorAt(1.0, m_recording ? QColor(210, 29, 45) : QColor(10, 91, 238));

    const qreal g = d * 0.126;
    painter.setPen(Qt::NoPen);
    painter.setBrush(blue);
    painter.drawEllipse(bounds.adjusted(g, g, -g, -g));

    painter.setBrush(m_recording ? QColor(255, 255, 255) : QColor(239, 48, 57));
    if (m_recording) {
        const qreal s = d * 0.109;
        painter.drawRoundedRect(QRectF(center.x() - s, center.y() - s, s * 2, s * 2), 4, 4);
    } else {
        painter.drawEllipse(center, d * 0.172, d * 0.172);
    }

    if (hasFocus()) {
        QPen focusPen(QColor(9, 103, 242), 2);
        painter.setPen(focusPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(bounds.adjusted(2, 2, -2, -2));
    }
}
