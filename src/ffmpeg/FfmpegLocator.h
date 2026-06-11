#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

namespace ffmpeg_locator {

inline QString findFfmpegPath()
{
    const QString bundlePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(bundlePath))
        return bundlePath;

    const QString sourcePath = QString::fromUtf8(BLUECAP_SOURCE_DIR)
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(sourcePath))
        return sourcePath;

    return QStringLiteral("ffmpeg.exe");
}

inline QString sanitizeWindowTitle(const QString &title)
{
    QString sanitized = title;
    auto end = std::remove_if(sanitized.begin(), sanitized.end(), [](QChar c) {
        return c == '\n' || c == '\r' || c == '"' || c == '\'' || c == '\\'
            || c == ';' || c == '|' || c == '&' || c == '$' || c == '`' || c == '%';
    });
    if (end != sanitized.end())
        sanitized.chop(static_cast<int>(sanitized.end() - end));
    if (sanitized.length() > 1024)
        sanitized.truncate(1024);
    return sanitized;
}

}
