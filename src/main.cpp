#include "ui/MainWindow.h"

#include <QApplication>
#include <QFile>

namespace {

QString loadStyleSheet()
{
    QFile resourceStyle(QStringLiteral(":/bluecap.qss"));
    if (resourceStyle.open(QFile::ReadOnly | QFile::Text)) {
        return QString::fromUtf8(resourceStyle.readAll());
    }

    return {};
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

    const QString styleSheet = loadStyleSheet();
    if (!styleSheet.isEmpty()) {
        app.setStyleSheet(styleSheet);
    }

    MainWindow window;
    window.show();

    return app.exec();
}
