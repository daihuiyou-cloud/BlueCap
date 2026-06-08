#include "ui/MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QSettings>

namespace {

int detectSystemTheme()
{
    QSettings registry(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return registry.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0 ? 2 : 1;
}

int resolveTheme(int preference)
{
    if (preference == 0)
        return detectSystemTheme();
    return preference;
}

QString loadThemeStyleSheet(int theme)
{
    const QString path = (theme == 2)
        ? QStringLiteral(":/bluecap-dark.qss")
        : QStringLiteral(":/bluecap.qss");
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text))
        return QString::fromUtf8(file.readAll());
    return {};
}

void applyTheme(int preference)
{
    const int theme = resolveTheme(preference);
    const QString qss = loadThemeStyleSheet(theme);
    if (!qss.isEmpty())
        qApp->setStyleSheet(qss);
}

}

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueCap"));
    app.setOrganizationName(QStringLiteral("BlueCap"));
    app.setQuitOnLastWindowClosed(false);

    QSettings settings;
    const int preference = settings.value(QStringLiteral("settings/theme"), 0).toInt();
    applyTheme(preference);

    MainWindow window;
    window.show();

    return app.exec();
}
