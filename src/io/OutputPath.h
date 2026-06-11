#pragma once

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QString>

namespace output_path {

inline QString defaultSaveDir()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (baseDir.isEmpty())
        baseDir = QDir::homePath();
    return baseDir + QStringLiteral("/BlueCap");
}

inline QString generate(const QString &savePath)
{
    QString baseDir = savePath;
    if (baseDir.isEmpty())
        baseDir = defaultSaveDir();

    QDir dir(baseDir);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral(".")))
            return {};
    }

    const QString fileName = QStringLiteral("BlueCap_%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    return dir.filePath(fileName);
}

}
