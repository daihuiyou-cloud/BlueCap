#include "PaintedScrollBar.h"
#include "theme/ThemeColors.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

PaintedScrollBar::PaintedScrollBar(QWidget *parent)
    : QScrollBar(parent)
{
    setMouseTracking(true);
}

void PaintedScrollBar::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void PaintedScrollBar::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QScrollBar::enterEvent(event);
}

void PaintedScrollBar::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QScrollBar::leaveEvent(event);
}

void PaintedScrollBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = true;
    update();
    QScrollBar::mousePressEvent(event);
}

void PaintedScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_pressed = false;
    update();
    QScrollBar::mouseReleaseEvent(event);
}

void PaintedScrollBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    const int w = width();
    const int h = height();
    const int barW = 8;

    if (h <= 0) return;

    int range = maximum() - minimum() + pageStep();
    if (range <= 0) return;

    qreal handleH = qMax(30.0, qMin((qreal)h * 0.9, (qreal)h * pageStep() / range));
    qreal avail = h - handleH;
    qreal handleY = (maximum() <= minimum() || avail <= 0)
        ? 0
        : avail * (value() - minimum()) / (maximum() - minimum());

    QRectF handle((w - barW) / 2.0, handleY, barW, handleH);

    QColor color = m_pressed ? a.scrollbarHandlePressed
                 : m_hovered ? a.scrollbarHandleHover
                 : a.scrollbarHandle;

    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(handle, 4, 4);
}
