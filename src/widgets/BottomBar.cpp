#include "BottomBar.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>

BottomBar::BottomBar(QWidget *parent)
    : QFrame(parent)
{
    setMinimumHeight(74);
}

void BottomBar::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void BottomBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 10, 10);

    painter.setPen(QPen(a.bottomBarBorder, 1));
    painter.setBrush(a.bottomBarBg);
    painter.drawPath(path);
}
