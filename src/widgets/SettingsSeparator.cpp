#include "SettingsSeparator.h"
#include "theme/ThemeColors.h"

#include <QPainter>

SettingsSeparator::SettingsSeparator(QWidget *parent)
    : QFrame(parent)
{
    setFixedHeight(1);
}

void SettingsSeparator::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

void SettingsSeparator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const auto &a = ThemeColors::forMode(m_darkMode).app;
    painter.setPen(Qt::NoPen);
    painter.setBrush(a.settingsSeparator);
    painter.drawRect(rect());
}
