#include "style/BlueCapStyle.h"
#include "theme/Theme.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("BlueCap"));
    app.setOrganizationName(QStringLiteral("BlueCap"));
    app.setQuitOnLastWindowClosed(false);

    app.setStyle(new BlueCapStyle);

    QFont appFont = app.font();
    appFont.setFamilies({QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")});
    app.setFont(appFont);

    QSettings settings;
    theme::apply(settings.value(QStringLiteral("settings/theme"), ThemeSystem).toInt());

    MainWindow window;
    window.show();

    return app.exec();
}
