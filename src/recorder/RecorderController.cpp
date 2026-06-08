#include "RecorderController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

RecorderController::RecorderController(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &RecorderController::handleFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &RecorderController::handleProcessError);
}

bool RecorderController::isRecording() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString RecorderController::currentOutputPath() const
{
    return m_currentOutputPath;
}

void RecorderController::startFullScreenRecording()
{
    if (isRecording()) {
        return;
    }

    const QString ffmpegPath = resolveFfmpegPath();
    if (!QFileInfo::exists(ffmpegPath)) {
        emit errorOccurred(QStringLiteral("未找到 FFmpeg：%1").arg(ffmpegPath));
        return;
    }

    m_currentOutputPath = createOutputPath();
    m_stopRequested = false;

    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("gdigrab"),
        QStringLiteral("-framerate"), QStringLiteral("30"),
        QStringLiteral("-i"), QStringLiteral("desktop"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), QStringLiteral("ultrafast"),
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        m_currentOutputPath
    };

    emit outputPathChanged(m_currentOutputPath);
    m_process->start(ffmpegPath, args);

    if (!m_process->waitForStarted(1500)) {
        emit errorOccurred(QStringLiteral("FFmpeg 启动失败：%1").arg(m_process->errorString()));
        return;
    }

    emit recordingChanged(true);
}

void RecorderController::stopRecording()
{
    if (!isRecording()) {
        return;
    }

    m_stopRequested = true;
    m_process->write("q\n");
    m_process->waitForBytesWritten(500);

    if (!m_process->waitForFinished(3000)) {
        m_process->terminate();
        if (!m_process->waitForFinished(1500)) {
            m_process->kill();
        }
    }
}

void RecorderController::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    emit recordingChanged(false);

    if (status == QProcess::NormalExit && (exitCode == 0 || m_stopRequested)
        && QFileInfo::exists(m_currentOutputPath)) {
        emit videoSaved(m_currentOutputPath);
        return;
    }

    const QString ffmpegOutput = QString::fromLocal8Bit(m_process->readAll()).trimmed();
    emit errorOccurred(ffmpegOutput.isEmpty()
        ? QStringLiteral("录制进程异常退出。")
        : ffmpegOutput);
}

void RecorderController::handleProcessError(QProcess::ProcessError)
{
    if (!m_stopRequested) {
        emit errorOccurred(m_process->errorString());
    }
}

QString RecorderController::resolveFfmpegPath() const
{
    const QString runtimePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(runtimePath)) {
        return runtimePath;
    }

    return QString::fromUtf8(BLUECAP_SOURCE_DIR) + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
}

QString RecorderController::createOutputPath() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }

    QDir dir(baseDir + QStringLiteral("/BlueCap"));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString fileName = QStringLiteral("BlueCap_%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    return dir.filePath(fileName);
}
