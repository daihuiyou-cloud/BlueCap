#include "RecordButton.h"
#include "paint/PaintMetrics.h"

#include <QMouseEvent>
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
    return QSize(paint::Metrics::recordButtonSize, paint::Metrics::recordButtonSize);
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

void RecordButton::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void RecordButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void RecordButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
    QAbstractButton::mousePressEvent(event);
}

void RecordButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = false;
    update();
    QAbstractButton::mouseReleaseEvent(event);
}

void RecordButton::renderCache()
{
    m_bgCache = QPixmap(size());
    m_bgCache.fill(Qt::transparent);

    QPainter p(&m_bgCache);
    p.setRenderHint(QPainter::Antialiasing);

    const int m = paint::Metrics::recordButtonMargin;
    const QRectF bounds = rect().adjusted(m, m, -m, -m);
    const QPointF center = bounds.center();
    const qreal d = qMin(bounds.width(), bounds.height());
    const qreal r = d / 2.0;

    QRadialGradient shadow(center + QPointF(0, d * 0.12), r * 1.2);
    int shadowAlpha = m_darkMode ? 34 : 58;
    shadow.setColorAt(0.2, QColor(36, 100, 220, shadowAlpha));
    shadow.setColorAt(1.0, QColor(36, 100, 220, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(shadow);
    p.drawEllipse(center, r * 1.08, r * 1.08);

    p.setBrush(m_darkMode ? QColor(43, 51, 66) : QColor(246, 250, 255));
    p.drawEllipse(bounds);

    QPen rim(m_darkMode ? QColor(65, 76, 96) : QColor(225, 234, 248), 1.2);
    p.setPen(rim);
    p.setBrush(Qt::NoBrush);
    qreal hw = 1.2 / 2.0;
    p.drawEllipse(bounds.adjusted(hw, hw, -hw, -hw));
}

void RecordButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const bool disabled = !isEnabled();
    const qreal disabledOpacity = disabled ? 0.45 : 1.0;
    if (disabledOpacity < 1.0)
        painter.setOpacity(disabledOpacity);

    if (!m_bgCache.isNull())
        painter.drawPixmap(0, 0, m_bgCache);

    const int m = paint::Metrics::recordButtonMargin;
    const QRectF bounds = rect().adjusted(m, m, -m, -m);
    const QPointF center = bounds.center();
    const qreal d = qMin(bounds.width(), bounds.height());

    qreal pressScale = m_pressed ? 0.96 : 1.0;
    qreal gBase = d * 0.135;
    if (pressScale < 1.0) {
        painter.save();
        painter.translate(center);
        painter.scale(pressScale, pressScale);
        painter.translate(-center);
    }

    QLinearGradient blue(bounds.topLeft(), bounds.bottomRight());
    if (disabled) {
        blue.setColorAt(0.0, QColor(160, 160, 160));
        blue.setColorAt(1.0, QColor(120, 120, 120));
    } else if (m_recording) {
        if (m_pressed) {
            blue.setColorAt(0.0, QColor(210, 60, 60));
            blue.setColorAt(1.0, QColor(170, 18, 30));
        } else if (m_hovered) {
            blue.setColorAt(0.0, QColor(255, 110, 110));
            blue.setColorAt(1.0, QColor(220, 40, 56));
        } else {
            blue.setColorAt(0.0, QColor(255, 89, 89));
            blue.setColorAt(1.0, QColor(210, 29, 45));
        }
    } else {
        if (m_pressed) {
            blue.setColorAt(0.0, QColor(55, 135, 220));
            blue.setColorAt(1.0, QColor(8, 75, 210));
        } else if (m_hovered) {
            blue.setColorAt(0.0, QColor(92, 178, 255));
            blue.setColorAt(1.0, QColor(26, 104, 245));
        } else {
            blue.setColorAt(0.0, QColor(77, 166, 255));
            blue.setColorAt(1.0, QColor(10, 91, 238));
        }
    }

    const qreal g = m_pressed ? gBase * 1.1 : gBase;
    painter.setPen(Qt::NoPen);
    painter.setBrush(blue);
    painter.drawEllipse(bounds.adjusted(g, g, -g, -g));

    if (!disabled) {
        QPen highlight(QColor(255, 255, 255, m_darkMode ? 42 : 70), 1.4);
        painter.setPen(highlight);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(bounds.adjusted(g + 1.2, g + 1.2, -g - 1.2, -g - 1.2));
    }

    QColor iconColor;
    if (disabled) {
        iconColor = QColor(180, 180, 180);
    } else if (m_recording) {
        iconColor = m_darkMode ? QColor(220, 220, 220) : QColor(255, 255, 255);
    } else {
        iconColor = QColor(239, 48, 57);
    }
    painter.setBrush(iconColor);

    if (m_recording) {
        const qreal s = d * 0.109;
        painter.drawRoundedRect(QRectF(center.x() - s, center.y() - s, s * 2, s * 2), 4, 4);
    } else {
        painter.drawEllipse(center, d * 0.172, d * 0.172);
    }

    if (pressScale < 1.0)
        painter.restore();

    if (hasFocus() && !m_pressed) {
        QPen focusPen(QColor(9, 103, 242), 2);
        painter.setPen(focusPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(bounds.adjusted(2, 2, -2, -2));
    }
}
