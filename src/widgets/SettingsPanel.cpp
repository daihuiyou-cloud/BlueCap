#include "SettingsPanel.h"
#include "paint/PaintMetrics.h"
#include "theme/ThemeColors.h"

#include <QPainter>
#include <QPainterPath>

SettingsPanel::SettingsPanel(QWidget *parent)
    : QFrame(parent)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMaximumWidth(paint::Metrics::settingsPanelMaxWidth);
}

void SettingsPanel::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void SettingsPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const auto &a = ThemeColors::forMode(m_darkMode).app;
    QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 12, 12);

    painter.setPen(QPen(a.settingsPanelBorder, 1));
    painter.setBrush(a.settingsPanelBg);
    painter.drawPath(path);
}
