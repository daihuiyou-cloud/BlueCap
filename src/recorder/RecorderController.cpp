#include "RecorderController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

#include <windows.h>

namespace {
constexpr int kMaxStderrBuffer = 65536;

QString friendlyError(const QString &raw)
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

QString sanitizeWindowTitle(const QString &title)
{
    QString sanitized = title;
    sanitized.remove(QChar('\n'));
    sanitized.remove(QChar('\r'));
    sanitized.remove(QChar('"'));
    sanitized.remove(QChar('\''));
    sanitized.remove(QChar('\\'));
    sanitized.remove(QChar(';'));
    sanitized.remove(QChar('|'));
    sanitized.remove(QChar('&'));
    sanitized.remove(QChar('$'));
    sanitized.remove(QChar('`'));
    sanitized.remove(QChar('%'));
    if (sanitized.length() > 1024)
        sanitized = sanitized.left(1024);
    return sanitized;
}

QString processErrorToString(QProcess::ProcessError error)
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

RecorderController::RecorderController(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::started, this, &RecorderController::handleStarted);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &RecorderController::handleFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &RecorderController::handleProcessError);
    connect(m_process, &QProcess::readyReadStandardError, this, [this] {
        m_stderrBuffer.append(m_process->readAllStandardError());
        if (m_stderrBuffer.size() > kMaxStderrBuffer)
            m_stderrBuffer = m_stderrBuffer.right(kMaxStderrBuffer);
    });

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    connect(m_startTimer, &QTimer::timeout, this, &RecorderController::handleStartTimeout);

    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &RecorderController::handleStopTimeout);

    QTimer::singleShot(0, this, &RecorderController::detectHardwareEncoderAsync);
}

bool RecorderController::isRecording() const
{
    return m_recording;
}

QString RecorderController::currentOutputPath() const
{
    return m_currentOutputPath;
}

QString RecorderController::currentSavePath() const
{
    if (!m_savePath.isEmpty())
        return m_savePath;
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (baseDir.isEmpty())
        baseDir = QDir::homePath();
    return baseDir + QStringLiteral("/BlueCap");
}

void RecorderController::setFrameRate(int fps)
{
    m_frameRate = qBound(1, fps, 60);
}

void RecorderController::setPreset(const QString &preset)
{
    m_preset = preset;
}

void RecorderController::setShowCursor(bool show)
{
    m_showCursor = show;
}

void RecorderController::setStartTimeout(int ms)
{
    m_startTimeoutMs = ms;
}

void RecorderController::setSavePath(const QString &path)
{
    m_savePath = path;
}

void RecorderController::setStopTimeout(int ms)
{
    m_stopTimeoutMs = ms;
}

QList<RecorderController::WindowEntry> RecorderController::enumerateWindows()
{
    struct WindowInfo {
        QString title;
        HWND hwnd;
    };
    QList<WindowInfo> windows;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        if (!IsWindowVisible(hwnd) || !GetWindowTextLengthW(hwnd)) {
            return TRUE;
        }
        wchar_t buf[256];
        GetWindowTextW(hwnd, buf, 256);
        auto *list = reinterpret_cast<QList<WindowInfo>*>(lParam);
        list->append({ QString::fromWCharArray(buf), hwnd });
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));

    QMap<QString, int> counts;
    QList<WindowEntry> result;
    for (const auto &info : windows) {
        const QString &original = info.title;
        QString display = original;
        int &count = counts[original];
        if (count > 0) {
            display += QStringLiteral(" (%1)").arg(count + 1);
        }
        result.append({ display, original, reinterpret_cast<qulonglong>(info.hwnd) });
        count++;
    }
    return result;
}

void RecorderController::startCapture(const QString &inputSpec, const QStringList &extraArgs)
{
    if (isRecording()) return;

    const QString ffmpegPath = resolveFfmpegPath();
    if (!QFileInfo::exists(ffmpegPath)) {
        emit errorOccurred(QStringLiteral("未找到录制程序 (ffmpeg.exe)，请确认文件存在。"));
        return;
    }

    m_currentOutputPath = createOutputPath();
    if (m_currentOutputPath.isEmpty()) {
        emit errorOccurred(QStringLiteral("无法创建保存目录，请检查权限或磁盘状态。"));
        return;
    }
    m_stopRequested = false;
    m_forceKilled = false;
    m_errorReported = false;

    QString saveDir = QFileInfo(m_currentOutputPath).absolutePath();
    ULARGE_INTEGER freeBytes;
    if (GetDiskFreeSpaceExW(saveDir.toStdWString().c_str(), &freeBytes, nullptr, nullptr)
        && freeBytes.QuadPart < 200LL * 1024 * 1024) {
        emit errorOccurred(QStringLiteral("磁盘空间不足（剩余不足 200 MB），请释放空间后重试。"));
        return;
    }

    if (!m_encoderDetected)
        m_encoder = QStringLiteral("libx264");

    QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("gdigrab"),
        QStringLiteral("-framerate"), QString::number(m_frameRate),
        QStringLiteral("-i"), inputSpec,
        QStringLiteral("-c:v"), m_encoder,
    };
    if (m_encoder == QStringLiteral("libx264"))
        args << QStringLiteral("-preset") << m_preset;
    args << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    if (!m_showCursor)
        args << QStringLiteral("-draw_mouse") << QStringLiteral("0");
    args << extraArgs << m_currentOutputPath;

    start(args);
}

void RecorderController::startFullScreenRecording()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    emit recordingAreaChanged(screen->geometry(), RecordMode::FullScreen);
    startCapture(QStringLiteral("desktop"));
}

void RecorderController::startRegionRecording(const QRect &region)
{
    emit recordingAreaChanged(region, RecordMode::Region);
    const QString cropFilter = QStringLiteral("crop=%1:%2:%3:%4")
        .arg(region.width()).arg(region.height())
        .arg(region.x()).arg(region.y());
    startCapture(QStringLiteral("desktop"), { QStringLiteral("-vf"), cropFilter });
}

void RecorderController::startWindowRecording(const QString &windowTitle)
{
    startCapture(QStringLiteral("title=%1").arg(sanitizeWindowTitle(windowTitle)));
}

void RecorderController::start(const QStringList &args)
{
    emit outputPathChanged(m_currentOutputPath);
    m_process->start(m_ffmpegPath, args);
    m_startTimer->start(m_startTimeoutMs);
}

void RecorderController::stopRecording()
{
    if (!isRecording()) return;

    m_stopRequested = true;
    if (m_process->write("q\n") == -1) {
        m_process->kill();
        return;
    }
    m_stopTimer->start(m_stopTimeoutMs);
}

void RecorderController::handleStarted()
{
    m_startTimer->stop();
    m_recording = true;
    emit recordingChanged(true);
}

void RecorderController::handleFinished(int exitCode, QProcess::ExitStatus)
{
    m_startTimer->stop();
    m_stopTimer->stop();
    m_recording = false;
    m_exitCode = exitCode;

    emit recordingChanged(false);

    QTimer::singleShot(200, this, &RecorderController::handleFinishedCheck);
}

void RecorderController::handleFinishedCheck()
{
    const QFileInfo fi(m_currentOutputPath);
    const bool fileExists = fi.exists();
    const bool fileNonEmpty = fileExists && fi.size() > 0;

    if (m_errorReported) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        return;
    }

    if (m_forceKilled) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        emit errorOccurred(QStringLiteral("录制停止超时，已强制终止。输出文件可能不完整，已自动删除。"));
        return;
    }

    if (m_exitCode != 0 && !m_stopRequested) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        const QString ffmpegOutput = QString::fromLocal8Bit(m_stderrBuffer).trimmed();
        m_stderrBuffer.clear();
        emit errorOccurred(ffmpegOutput.isEmpty()
            ? QStringLiteral("录制异常退出（错误码 %1），请重试。").arg(m_exitCode)
            : friendlyError(ffmpegOutput));
        return;
    }

    if (fileNonEmpty) {
        emit videoSaved(m_currentOutputPath);
        return;
    }

    const QString ffmpegOutput = QString::fromLocal8Bit(m_stderrBuffer).trimmed();
    m_stderrBuffer.clear();
    emit errorOccurred(ffmpegOutput.isEmpty()
        ? QStringLiteral("录制意外中断，请重试。")
        : friendlyError(ffmpegOutput));
}

void RecorderController::handleProcessError(QProcess::ProcessError error)
{
    m_startTimer->stop();
    m_errorReported = true;
    if (!m_stopRequested) {
        emit errorOccurred(processErrorToString(error));
    }
}

void RecorderController::handleStartTimeout()
{
    m_recording = false;
    m_process->kill();
    emit errorOccurred(QStringLiteral("录制程序启动超时，请检查系统资源后重试。"));
    emit recordingChanged(false);
}

void RecorderController::handleStopTimeout()
{
    m_forceKilled = true;
    m_process->terminate();
    // Safety net: force-report failure after kill attempt
    QPointer<RecorderController> guard(this);
    QTimer::singleShot(2000, this, [this, guard] {
        if (!guard) return;
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
        // If the process is still alive after kill, force-report (handleFinished won't fire)
        QTimer::singleShot(500, this, [this, guard] {
            if (!guard) return;
            if (m_process->state() != QProcess::NotRunning) {
                emit recordingChanged(false);
                emit errorOccurred(QStringLiteral("录制停止超时，已强制终止。输出文件可能不完整。"));
            }
            // If process is NotRunning, handleFinished already handled it — skip
        });
    });
}

void RecorderController::detectHardwareEncoderAsync()
{
    const QString ffmpeg = resolveFfmpegPath();
    if (!QFileInfo::exists(ffmpeg)) {
        m_encoder = QStringLiteral("libx264");
        m_encoderDetected = true;
        return;
    }

    auto *probe = new QProcess(this);
    probe->setProcessChannelMode(QProcess::MergedChannels);
    connect(probe, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        this, [this, probe](int, QProcess::ExitStatus) {
        const QString output = QString::fromLocal8Bit(probe->readAllStandardOutput());
        if (output.contains(QStringLiteral("h264_mf")))
            m_encoder = QStringLiteral("h264_mf");
        else if (output.contains(QStringLiteral("h264_nvenc")))
            m_encoder = QStringLiteral("h264_nvenc");
        else if (output.contains(QStringLiteral("h264_amf")))
            m_encoder = QStringLiteral("h264_amf");
        else if (output.contains(QStringLiteral("h264_qsv")))
            m_encoder = QStringLiteral("h264_qsv");
        else
            m_encoder = QStringLiteral("libx264");
        m_encoderDetected = true;
        probe->deleteLater();
    });
    connect(probe, &QProcess::errorOccurred, this, [this, probe] {
        m_encoder = QStringLiteral("libx264");
        m_encoderDetected = true;
        probe->deleteLater();
    });
    probe->start(ffmpeg, { QStringLiteral("-hide_banner"), QStringLiteral("-encoders") });
    QTimer::singleShot(5000, probe, [probe] {
        if (probe->state() != QProcess::NotRunning) {
            probe->kill();
        }
    });
}

QString RecorderController::detectHardwareEncoder()
{
    const QString ffmpeg = resolveFfmpegPath();
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(ffmpeg, { QStringLiteral("-hide_banner"), QStringLiteral("-encoders") });
    if (!probe.waitForFinished(5000)) {
        probe.kill();
        return QStringLiteral("libx264");
    }
    const QString output = QString::fromLocal8Bit(probe.readAllStandardOutput());

    // h264_mf (MediaFoundation) is built into Windows, always reliable
    if (output.contains(QStringLiteral("h264_mf")))
        return QStringLiteral("h264_mf");
    if (output.contains(QStringLiteral("h264_nvenc")))
        return QStringLiteral("h264_nvenc");
    if (output.contains(QStringLiteral("h264_amf")))
        return QStringLiteral("h264_amf");
    if (output.contains(QStringLiteral("h264_qsv")))
        return QStringLiteral("h264_qsv");

    return QStringLiteral("libx264");
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
    QString baseDir = m_savePath;
    if (baseDir.isEmpty()) {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        if (baseDir.isEmpty()) {
            baseDir = QDir::homePath();
        }
        baseDir += QStringLiteral("/BlueCap");
    }

    QDir dir(baseDir);
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            return {};
        }
    }

    const QString fileName = QStringLiteral("BlueCap_%1.mp4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    return dir.filePath(fileName);
}
