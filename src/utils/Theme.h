#pragma once

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QString>

enum ThemePreference {
    ThemeSystem = 0,
    ThemeLight  = 1,
    ThemeDark   = 2
};

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

inline QString loadStyleSheet(int theme)
{
    const QString path = (theme == ThemeDark)
        ? QStringLiteral(":/bluecap-dark.qss")
        : QStringLiteral(":/bluecap.qss");
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text))
        return QString::fromUtf8(file.readAll());
    return {};
}

inline void apply(int preference)
{
    const int theme = resolve(preference);
    const QString qss = loadStyleSheet(theme);
    if (!qss.isEmpty())
        qApp->setStyleSheet(qss);
}

}
