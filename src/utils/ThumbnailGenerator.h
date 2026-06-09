#pragma once

#include <QImage>
#include <QProcess>
#include <QString>

#include "FfmpegLocator.h"

namespace thumbnail {

inline QImage fromVideo(const QString &filePath)
{
    const QString ffmpeg = ffmpeg_locator::findFfmpegPath();
    if (ffmpeg.isEmpty())
        return {};

    QProcess proc;
    proc.setProcessChannelMode(QProcess::SeparateChannels);
    proc.start(ffmpeg, {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-ss"), QStringLiteral("00:00:00"),
        QStringLiteral("-i"), filePath,
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-vf"), QStringLiteral("scale=320:180:force_original_aspect_ratio=increase,crop=320:180"),
        QStringLiteral("-f"), QStringLiteral("image2pipe"),
        QStringLiteral("-vcodec"), QStringLiteral("png"),
        QStringLiteral("pipe:1")
    });
    if (!proc.waitForFinished(15000) || proc.exitCode() != 0)
        return {};

    QImage image;
    image.loadFromData(proc.readAllStandardOutput(), "PNG");
    return image;
}

}
