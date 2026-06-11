#include "SurfaceWidget.h"
#include "paint/PaintMetrics.h"
#include "theme/ThemeColors.h"

#include <QPainter>

SurfaceWidget::SurfaceWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
}

void SurfaceWidget::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void SurfaceWidget::setRoundedCorners(bool enabled)
{
    m_roundedCorners = enabled;
    update();
}

void SurfaceWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(a.surfaceBorder, 1));
    painter.setBrush(a.surfaceBg);
    int radius = m_roundedCorners ? paint::Metrics::windowRadius : 0;
    painter.drawRoundedRect(r, radius, radius);
}
