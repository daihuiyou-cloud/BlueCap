#include "RecorderController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

#include <windows.h>

RecorderController::RecorderController(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::started, this, &RecorderController::handleStarted);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &RecorderController::handleFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &RecorderController::handleProcessError);

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    connect(m_startTimer, &QTimer::timeout, this, &RecorderController::handleStartTimeout);

    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &RecorderController::handleStopTimeout);
}

bool RecorderController::isRecording() const
{
    return m_process->state() != QProcess::NotRunning;
}

QString RecorderController::currentOutputPath() const
{
    return m_currentOutputPath;
}

void RecorderController::setFrameRate(int fps)
{
    m_frameRate = fps;
}

void RecorderController::setPreset(const QString &preset)
{
    m_preset = preset;
}

QStringList RecorderController::enumerateWindows()
{
    QSet<QString> titles;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!IsWindowVisible(hwnd) || !GetWindowTextLengthW(hwnd)) {
            return TRUE;
        }
        wchar_t buf[256];
        GetWindowTextW(hwnd, buf, 256);
        reinterpret_cast<QSet<QString>*>(lParam)->insert(QString::fromWCharArray(buf));
        return TRUE;
    }, reinterpret_cast<LPARAM>(&titles));

    return titles.values();
}

void RecorderController::startFullScreenRecording()
{
    if (isRecording()) return;

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
        QStringLiteral("-framerate"), QString::number(m_frameRate),
        QStringLiteral("-i"), QStringLiteral("desktop"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), m_preset,
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        m_currentOutputPath
    };

    start(args);
}

void RecorderController::startRegionRecording(const QRect &region)
{
    if (isRecording()) return;

    const QString ffmpegPath = resolveFfmpegPath();
    if (!QFileInfo::exists(ffmpegPath)) {
        emit errorOccurred(QStringLiteral("未找到 FFmpeg：%1").arg(ffmpegPath));
        return;
    }

    m_currentOutputPath = createOutputPath();
    m_stopRequested = false;

    const QString videoSize = QStringLiteral("%1x%2")
        .arg(region.width()).arg(region.height());
    const QString offset = QStringLiteral("%1,%2")
        .arg(region.x()).arg(region.y());

    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("gdigrab"),
        QStringLiteral("-framerate"), QString::number(m_frameRate),
        QStringLiteral("-offset_x"), QString::number(region.x()),
        QStringLiteral("-offset_y"), QString::number(region.y()),
        QStringLiteral("-video_size"), videoSize,
        QStringLiteral("-i"), QStringLiteral("desktop"),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), m_preset,
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        m_currentOutputPath
    };

    start(args);
}

void RecorderController::startWindowRecording(const QString &windowTitle)
{
    if (isRecording()) return;

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
        QStringLiteral("-framerate"), QString::number(m_frameRate),
        QStringLiteral("-i"), QStringLiteral("title=%1").arg(windowTitle),
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), m_preset,
        QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"),
        m_currentOutputPath
    };

    start(args);
}

void RecorderController::start(const QStringList &args)
{
    emit outputPathChanged(m_currentOutputPath);
    m_process->start(m_ffmpegPath, args);
    m_startTimer->start(3000);
}

void RecorderController::stopRecording()
{
    if (!isRecording()) return;

    m_stopRequested = true;
    m_process->write("q\n");
    m_stopTimer->start(3000);
}

void RecorderController::handleStarted()
{
    m_startTimer->stop();
    emit recordingChanged(true);
}

void RecorderController::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startTimer->stop();
    m_stopTimer->stop();

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
    m_startTimer->stop();
    if (!m_stopRequested) {
        emit errorOccurred(m_process->errorString());
    }
}

void RecorderController::handleStartTimeout()
{
    m_process->kill();
    emit errorOccurred(QStringLiteral("FFmpeg 启动超时"));
    emit recordingChanged(false);
}

void RecorderController::handleStopTimeout()
{
    m_process->terminate();
    QTimer::singleShot(1500, this, [this] {
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    });
}

QString RecorderController::resolveFfmpegPath()
{
    if (!m_ffmpegPath.isEmpty())
        return m_ffmpegPath;

    const QString bundlePath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(bundlePath)) {
        m_ffmpegPath = bundlePath;
        return m_ffmpegPath;
    }

    const QString sourcePath = QString::fromUtf8(BLUECAP_SOURCE_DIR)
        + QStringLiteral("/3rd/ffmpeg/ffmpeg.exe");
    if (QFileInfo::exists(sourcePath)) {
        m_ffmpegPath = sourcePath;
        return m_ffmpegPath;
    }

    m_ffmpegPath = QStringLiteral("ffmpeg.exe");
    return m_ffmpegPath;
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
