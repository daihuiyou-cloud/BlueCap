#pragma once

#include "ThemeColors.h"
#include "ThemePreference.h"
#include "style/BlueCapStyle.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QString>

namespace theme {

inline int detectSystem()
{
    QSettings registry(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return registry.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0 ? ThemeDark : ThemeLight;
}

inline int resolve(int preference)
{
    return preference == ThemeSystem ? detectSystem() : preference;
}

inline void apply(int preference, QApplication *app = qApp)
{
    const int th = resolve(preference);
    bool dark = (th == ThemeDark);
    const auto &a = ThemeColors::forMode(dark).app;
    QPalette pal = app->palette();
    pal.setColor(QPalette::WindowText, a.defaultText);
    app->setPalette(pal);

    auto *style = qobject_cast<BlueCapStyle *>(app->style());
    if (style) {
        style->setDarkMode(dark);
        app->setStyleSheet(QString());
        app->style()->unpolish(app);
        app->style()->polish(app);
    }
}

}
