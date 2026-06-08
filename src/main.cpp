#include "ui/MainWindow.h"
#include "utils/Theme.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueCap"));
    app.setOrganizationName(QStringLiteral("BlueCap"));
    app.setQuitOnLastWindowClosed(false);

    QSettings settings;
    theme::apply(settings.value(QStringLiteral("settings/theme"), ThemeSystem).toInt());

    MainWindow window;
    window.show();

    return app.exec();
}
