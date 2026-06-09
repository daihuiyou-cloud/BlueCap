#pragma once

#include <QFileInfo>
#include <QProcess>
#include <QString>

namespace ffmpeg_encoder {

inline QString detect(const QString &ffmpegPath)
{
    if (!QFileInfo::exists(ffmpegPath))
        return QStringLiteral("libx264");

    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(ffmpegPath, { QStringLiteral("-hide_banner"), QStringLiteral("-encoders") });

    if (probe.waitForFinished(5000) && probe.exitCode() == 0) {
        const QString output = QString::fromUtf8(probe.readAllStandardOutput());
        if (output.contains(QStringLiteral("h264_mf")))
            return QStringLiteral("h264_mf");
        if (output.contains(QStringLiteral("h264_nvenc")))
            return QStringLiteral("h264_nvenc");
        if (output.contains(QStringLiteral("h264_amf")))
            return QStringLiteral("h264_amf");
        if (output.contains(QStringLiteral("h264_qsv")))
            return QStringLiteral("h264_qsv");
    }

    return QStringLiteral("libx264");
}

}
