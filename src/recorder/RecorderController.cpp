#include "RecorderController.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QMap>
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
        QByteArray chunk = m_process->readAllStandardError();
        m_stderrSize += chunk.size();
        m_stderrChunks.push_back(chunk);
        while (m_stderrSize > kMaxStderrBuffer && !m_stderrChunks.empty()) {
            m_stderrSize -= m_stderrChunks.front().size();
            m_stderrChunks.pop_front();
        }

        m_stderrPending.append(chunk);
        if (m_stderrPending.size() > 4096)
            m_stderrPending = m_stderrPending.right(2048);
    });

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    connect(m_startTimer, &QTimer::timeout, this, &RecorderController::handleStartTimeout);

    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &RecorderController::handleStopTimeout);

    m_stderrMonitor = new QTimer(this);
    m_stderrMonitor->setInterval(5000);
    connect(m_stderrMonitor, &QTimer::timeout, this, &RecorderController::monitorStderr);

    detectHardwareEncoder();
}

void RecorderController::detectHardwareEncoder()
{
    const QString ffmpeg = resolveFfmpegPath();
    if (!QFileInfo::exists(ffmpeg)) {
        m_encoder = QStringLiteral("libx264");
        m_encoderDetected = true;
        return;
    }

    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(ffmpeg, { QStringLiteral("-hide_banner"), QStringLiteral("-encoders") });
    if (probe.waitForFinished(5000) && probe.exitCode() == 0) {
        const QString output = QString::fromUtf8(probe.readAllStandardOutput());
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
    } else {
        m_encoder = QStringLiteral("libx264");
    }
    m_encoderDetected = true;
}

RecorderController::State RecorderController::state() const
{
    return m_state;
}

bool RecorderController::isRecording() const
{
    return m_state == State::Recording;
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
    static const QSet<QString> kAllowedPresets = {
        QStringLiteral("ultrafast"),
        QStringLiteral("superfast"),
        QStringLiteral("veryfast"),
        QStringLiteral("faster"),
        QStringLiteral("fast"),
        QStringLiteral("medium"),
        QStringLiteral("slow"),
    };
    m_preset = kAllowedPresets.contains(preset) ? preset : QStringLiteral("fast");
}

void RecorderController::setShowCursor(bool show)
{
    m_showCursor = show;
}

void RecorderController::setStartTimeout(int ms)
{
    m_startTimeoutMs = qBound(1000, ms, 30000);
}

void RecorderController::setSavePath(const QString &path)
{
    m_savePath = QDir::fromNativeSeparators(path.trimmed());
}

void RecorderController::setStopTimeout(int ms)
{
    m_stopTimeoutMs = qBound(1000, ms, 30000);
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

void RecorderController::startCapture(const QString &inputSpec,
                                      const QStringList &extraArgs,
                                      const QStringList &inputArgs)
{
    if (m_state != State::Idle) return;

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
    m_state = State::Starting;

    QString saveDir = QFileInfo(m_currentOutputPath).absolutePath();
    ULARGE_INTEGER freeBytes;
    if (GetDiskFreeSpaceExW(saveDir.toStdWString().c_str(), &freeBytes, nullptr, nullptr)
        && freeBytes.QuadPart < 200LL * 1024 * 1024) {
        m_state = State::Idle;
        emit errorOccurred(QStringLiteral("磁盘空间不足（剩余不足 200 MB），请释放空间后重试。"));
        return;
    }

    QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("gdigrab"),
    };
    // inputArgs (offset_x, offset_y, video_size, etc.) must go BEFORE -i
    args << inputArgs;
    if (!m_showCursor)
        args << QStringLiteral("-draw_mouse") << QStringLiteral("0");
    args << QStringLiteral("-framerate") << QString::number(m_frameRate);
    args << QStringLiteral("-i") << inputSpec;
    args << QStringLiteral("-c:v") << m_encoder;
    if (m_encoder == QStringLiteral("libx264"))
        args << QStringLiteral("-preset") << m_preset;
    args << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    args << extraArgs << m_currentOutputPath;

    start(args);
}

void RecorderController::startFullScreenRecording(QScreen *screen)
{
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    QRect geo = screen->geometry();
    qreal dpr = screen->devicePixelRatio();
    int physX = qRound(geo.x() * dpr);
    int physY = qRound(geo.y() * dpr);
    int physW = qRound(geo.width() * dpr);
    int physH = qRound(geo.height() * dpr);
    if (physW < 2) physW = 2;
    if (physH < 2) physH = 2;
    physW &= ~1;
    physH &= ~1;
    emit recordingAreaChanged(geo, RecordMode::FullScreen);
    startCapture(QStringLiteral("desktop"), {}, {
        QStringLiteral("-offset_x"), QString::number(physX),
        QStringLiteral("-offset_y"), QString::number(physY),
        QStringLiteral("-video_size"), QStringLiteral("%1x%2").arg(physW).arg(physH),
    });
}

void RecorderController::startRegionRecording(const QRect &region)
{
    emit recordingAreaChanged(region, RecordMode::Region);

    // Find the screen containing the region for DPI scaling
    QScreen *screen = QGuiApplication::primaryScreen();
    const auto screens = QGuiApplication::screens();
    for (auto *s : screens) {
        if (s->geometry().intersects(region)) {
            screen = s;
            break;
        }
    }

    // Convert from Qt logical pixels to gdigrab physical pixels
    QRect screenGeo = screen->geometry();
    qreal dpr = screen->devicePixelRatio();
    int localX = region.x() - screenGeo.x();
    int localY = region.y() - screenGeo.y();
    int physX = qRound(localX * dpr);
    int physY = qRound(localY * dpr);
    int physW = qRound(region.width() * dpr);
    int physH = qRound(region.height() * dpr);

    // yuv420p requires even width/height
    if (physW < 2) physW = 2;
    if (physH < 2) physH = 2;
    physW &= ~1;
    physH &= ~1;

    // Use gdigrab's native offset/video_size options instead of -vf crop
    // This captures only the needed region and avoids the crop filter entirely
    startCapture(QStringLiteral("desktop"), {}, {
        QStringLiteral("-offset_x"), QString::number(physX),
        QStringLiteral("-offset_y"), QString::number(physY),
        QStringLiteral("-video_size"), QStringLiteral("%1x%2").arg(physW).arg(physH),
    });
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
    if (m_state != State::Recording) return;

    m_state = State::Stopping;
    m_stopRequested = true;

    m_process->write("q\n");
    m_stopTimer->start(m_stopTimeoutMs);
}

void RecorderController::handleStarted()
{
    m_startTimer->stop();
    m_state = State::Recording;
    m_stderrPending.clear();
    m_reportedWarnings.clear();
    m_stderrMonitor->start();
    emit recordingChanged(true);
}

void RecorderController::handleFinished(int exitCode, QProcess::ExitStatus)
{
    m_startTimer->stop();
    m_stopTimer->stop();
    m_stderrMonitor->stop();
    m_state = State::Idle;
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

    if (m_exitCode != 0) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        QByteArray fullStderr;
        fullStderr.reserve(m_stderrSize);
        for (const auto &chunk : std::as_const(m_stderrChunks))
            fullStderr.append(chunk);
        const QString ffmpegOutput = QString::fromUtf8(fullStderr).trimmed();
        m_stderrChunks.clear();
        m_stderrSize = 0;
        emit errorOccurred(ffmpegOutput.isEmpty()
            ? QStringLiteral("录制异常退出（错误码 %1），请重试。").arg(m_exitCode)
            : friendlyError(ffmpegOutput));
        return;
    }

    if (fileNonEmpty) {
        emit videoSaved(m_currentOutputPath);
        return;
    }

    QByteArray fullStderr;
    fullStderr.reserve(m_stderrSize);
    for (const auto &chunk : std::as_const(m_stderrChunks))
        fullStderr.append(chunk);
    const QString ffmpegOutput = QString::fromUtf8(fullStderr).trimmed();
    m_stderrChunks.clear();
    m_stderrSize = 0;
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
    if (m_state != State::Starting) return;
    m_state = State::Idle;
    m_stderrMonitor->stop();
    m_process->kill();
    emit errorOccurred(QStringLiteral("录制程序启动超时，请检查系统资源后重试。"));
    emit recordingChanged(false);
}

void RecorderController::handleStopTimeout()
{
    if (m_process->state() == QProcess::NotRunning)
        return;

    m_process->terminate();
    if (m_process->waitForFinished(3000))
        return;

    m_forceKilled = true;
    m_process->kill();
    if (m_process->waitForFinished(2000))
        return;

    if (m_process->state() != QProcess::NotRunning) {
        m_state = State::Idle;
        emit recordingChanged(false);
        emit errorOccurred(QStringLiteral("录制停止超时，已强制终止。输出文件可能不完整。"));
    }
}

void RecorderController::monitorStderr()
{
    if (m_stderrPending.isEmpty())
        return;

    const QString text = QString::fromUtf8(m_stderrPending);
    m_stderrPending.clear();

    struct WarningPattern {
        QLatin1String pattern;
        QString message;
    };
    static const WarningPattern patterns[] = {
        { QLatin1String("dup="),   QStringLiteral("检测到重复/丢帧，系统负载可能过高") },
        { QLatin1String("drop="),  QStringLiteral("检测到丢帧，系统负载可能过高") },
        { QLatin1String("speed="), QStringLiteral("编码速度低于实时，建议降低帧率或画质") },
        { QLatin1String("too many packets buffered"), QStringLiteral("编码器缓冲积压，系统负载过高") },
        { QLatin1String("Invalid data found"),        QStringLiteral("屏幕数据异常，请检查是否有其他录屏程序") },
    };

    for (const auto &wp : patterns) {
        if (text.contains(wp.pattern) && !m_reportedWarnings.contains(wp.message)) {
            m_reportedWarnings.append(wp.message);
            emit recordingWarning(wp.message);
        }
    }
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
