#include "BottomNavSection.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>

BottomNavSection::BottomNavSection(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("bottomNavSection"));
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void BottomNavSection::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void BottomNavSection::enterEvent(QEvent *event)
{
    m_hovered = true;
    update();
    QFrame::enterEvent(event);
}

void BottomNavSection::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QFrame::leaveEvent(event);
}

void BottomNavSection::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_hovered) {
        const auto &a = ThemeColors::forMode(m_darkMode).app;
        painter.setPen(Qt::NoPen);
        painter.setBrush(a.bottomNavHoverBg);
        painter.drawRoundedRect(QRectF(rect()), 6, 6);
    }
}
