#include "RecorderController.h"
#include "RecordingSession.h"
#include "StderrMonitor.h"
#include "ffmpeg/FfmpegEncoderDetector.h"
#include "ffmpeg/FfmpegError.h"
#include "ffmpeg/FfmpegLocator.h"
#include "io/OutputPath.h"
#include "utils/win32/DiskSpaceChecker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>

RecorderController::RecorderController(QObject *parent)
    : IRecorderService(parent)
{
    m_session = new RecordingSession(this);
    m_stderrMonitor = new StderrMonitor(this);

    connect(m_session, &RecordingSession::started, this, [this] {
        m_stderrMonitor->start();
        emit recordingChanged(true);
    });

    connect(m_session, &RecordingSession::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        m_stderrMonitor->stop();
        Q_UNUSED(exitCode);
        emit recordingChanged(false);
        QTimer::singleShot(200, this, &RecorderController::handleFinishedCheck);
    });

    connect(m_session, &RecordingSession::processError, this, [this](QProcess::ProcessError error) {
        m_errorReported = true;
        if (!m_session->isRecording())
            emit errorOccurred(ffmpeg_error::processErrorToString(error));
    });

    connect(m_session, &RecordingSession::startTimeout, this, [this] {
        m_stderrMonitor->stop();
        emit errorOccurred(QStringLiteral("录制程序启动超时，请检查系统资源后重试。"));
        emit recordingChanged(false);
    });

    connect(m_session, &RecordingSession::stopTimeout, this, [this] {
        emit errorOccurred(QStringLiteral("录制停止超时，已强制终止。输出文件可能不完整。"));
        emit recordingChanged(false);
    });

    connect(m_session, &RecordingSession::stderrReady, this, [this] {
        m_stderrMonitor->feed(m_session->readStderr());
    });
    connect(m_stderrMonitor, &StderrMonitor::warningDetected,
            this, &IRecorderService::recordingWarning);

    m_encoder = ffmpeg_encoder::detect(ffmpeg_locator::findFfmpegPath());
}

bool RecorderController::isRecording() const
{
    return m_session->isRecording();
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

void RecorderController::startCapture(const QString &inputSpec,
                                      const QStringList &extraArgs,
                                      const QStringList &inputArgs)
{
    if (m_session->state() != RecordingSession::State::Idle) return;

    const QString ffmpegPath = ffmpeg_locator::findFfmpegPath();
    if (!QFileInfo::exists(ffmpegPath)) {
        emit errorOccurred(QStringLiteral("未找到录制程序 (ffmpeg.exe)，请确认文件存在。"));
        return;
    }

    m_currentOutputPath = output_path::generate(m_savePath);
    if (m_currentOutputPath.isEmpty()) {
        emit errorOccurred(QStringLiteral("无法创建保存目录，请检查权限或磁盘状态。"));
        return;
    }
    m_errorReported = false;

    QString saveDir = QFileInfo(m_currentOutputPath).absolutePath();
    if (!disk_space::hasAvailableSpace(saveDir, 200ULL * 1024 * 1024)) {
        emit errorOccurred(QStringLiteral("磁盘空间不足（剩余不足 200 MB），请释放空间后重试。"));
        return;
    }

    QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-f"), QStringLiteral("gdigrab"),
    };
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

namespace {

struct CapturePhys {
    int x, y, w, h;
};

CapturePhys toPhysical(const QRect &area, QScreen *screen)
{
    QRect screenGeo = screen->geometry();
    qreal dpr = screen->devicePixelRatio();
    int localX = area.x() - screenGeo.x();
    int localY = area.y() - screenGeo.y();
    int physX = qRound(localX * dpr);
    int physY = qRound(localY * dpr);
    int physW = qRound(area.width() * dpr);
    int physH = qRound(area.height() * dpr);
    if (physW < 2) physW = 2;
    if (physH < 2) physH = 2;
    return { physX, physY, physW & ~1, physH & ~1 };
}

QScreen *screenForRect(const QRect &region)
{
    for (auto *s : QGuiApplication::screens())
        if (s->geometry().intersects(region))
            return s;
    return QGuiApplication::primaryScreen();
}

}

void RecorderController::startFullScreenRecording(QScreen *screen)
{
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    QRect geo = screen->geometry();
    auto phys = toPhysical(geo, screen);
    emit recordingAreaChanged(geo, RecordMode::FullScreen);
    startCapture(QStringLiteral("desktop"), {}, {
        QStringLiteral("-offset_x"), QString::number(phys.x),
        QStringLiteral("-offset_y"), QString::number(phys.y),
        QStringLiteral("-video_size"), QStringLiteral("%1x%2").arg(phys.w).arg(phys.h),
    });
}

void RecorderController::startRegionRecording(const QRect &region)
{
    emit recordingAreaChanged(region, RecordMode::Region);
    auto phys = toPhysical(region, screenForRect(region));
    startCapture(QStringLiteral("desktop"), {}, {
        QStringLiteral("-offset_x"), QString::number(phys.x),
        QStringLiteral("-offset_y"), QString::number(phys.y),
        QStringLiteral("-video_size"), QStringLiteral("%1x%2").arg(phys.w).arg(phys.h),
    });
}

void RecorderController::startWindowRecording(const QString &windowTitle)
{
    startCapture(QStringLiteral("title=%1").arg(ffmpeg_locator::sanitizeWindowTitle(windowTitle)));
}

void RecorderController::start(const QStringList &args)
{
    emit outputPathChanged(m_currentOutputPath);
    m_session->start(ffmpeg_locator::findFfmpegPath(), args, m_startTimeoutMs);
}

void RecorderController::stopRecording()
{
    if (!m_session->isRecording()) return;

    m_session->writeStdin("q\n");
    m_session->stop(m_stopTimeoutMs);
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

    if (m_session->wasForceKilled()) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        emit errorOccurred(QStringLiteral("录制停止超时，已强制终止。输出文件可能不完整，已自动删除。"));
        return;
    }

    if (m_session->exitCode() != 0) {
        if (fileExists) QFile::remove(m_currentOutputPath);
        const QString ffmpegOutput = QString::fromUtf8(m_stderrMonitor->drain()).trimmed();
        emit errorOccurred(ffmpegOutput.isEmpty()
            ? QStringLiteral("录制异常退出（错误码 %1），请重试。").arg(m_session->exitCode())
            : ffmpeg_error::friendlyMessage(ffmpegOutput));
        return;
    }

    if (fileNonEmpty) {
        emit videoSaved(m_currentOutputPath);
        return;
    }

    const QString ffmpegOutput = QString::fromUtf8(m_stderrMonitor->drain()).trimmed();
    emit errorOccurred(ffmpegOutput.isEmpty()
        ? QStringLiteral("录制意外中断，请重试。")
        : ffmpeg_error::friendlyMessage(ffmpegOutput));
}


