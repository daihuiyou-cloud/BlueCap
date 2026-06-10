#pragma once

#include "ThemePreference.h"

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

}
