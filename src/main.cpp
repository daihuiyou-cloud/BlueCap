#include "ui/MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>

namespace {

QString loadStyleSheet()
{
    QFile resourceStyle(QStringLiteral(":/bluecap.qss"));
    if (resourceStyle.open(QFile::ReadOnly | QFile::Text)) {
        return QString::fromUtf8(resourceStyle.readAll());
    }

    QFile sourceStyle(QString::fromUtf8(BLUECAP_SOURCE_DIR) + QStringLiteral("/resources/bluecap.qss"));
    if (sourceStyle.open(QFile::ReadOnly | QFile::Text)) {
        return QString::fromUtf8(sourceStyle.readAll());
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
