#pragma once

#include <QProcess>
#include <QString>

namespace ffmpeg_error {

inline QString friendlyMessage(const QString &raw)
{
    if (raw.contains(QStringLiteral("gdigrab")) || raw.contains(QStringLiteral("desktop")))
        return QStringLiteral("屏幕捕获初始化失败，请检查是否有其他录屏程序在运行。");
    if (raw.contains(QStringLiteral("permission denied")) || raw.contains(QStringLiteral("access denied")))
        return QStringLiteral("没有文件写入权限，请检查保存路径。");
    if (raw.contains(QStringLiteral("No such file")))
        return QStringLiteral("未找到编码器，请检查录制程序配置。");
    if (raw.contains(QStringLiteral("Invalid argument")))
        return QStringLiteral("录制参数无效，请检查设置后重试。");
    if (raw.contains(QStringLiteral("Disk full")) || raw.contains(QStringLiteral("No space left")))
        return QStringLiteral("磁盘空间不足，请释放空间后重试。");
    return QStringLiteral("录制出现异常：%1\n\n请尝试重启应用。如果问题持续，请检查日志。").arg(raw);
}

inline QString processErrorToString(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        return QStringLiteral("录制程序启动失败，请确认 ffmpeg.exe 文件存在且未被占用。");
    case QProcess::Crashed:
        return QStringLiteral("录制程序意外崩溃，请重试。");
    case QProcess::Timedout:
        return QStringLiteral("录制操作超时。");
    default:
        return QStringLiteral("录制程序出现未知错误，请重试。");
    }
}

}
