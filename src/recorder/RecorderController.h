#pragma once

#include "RecordMode.h"

#include <deque>

#include <QObject>
#include <QByteArray>
#include <QProcess>
#include <QRect>
#include <QString>
#include <QStringList>

class QScreen;
class QTimer;

class RecorderController : public QObject
{
    Q_OBJECT

public:
    explicit RecorderController(QObject *parent = nullptr);

    enum class State { Idle, Starting, Recording, Stopping };
    State state() const;
    bool isRecording() const;
    QString currentOutputPath() const;
    QString currentSavePath() const;

    void setFrameRate(int fps);
    void setPreset(const QString &preset);
    void setSavePath(const QString &path);
    void setShowCursor(bool show);
    void setStartTimeout(int ms);
    void setStopTimeout(int ms);

public slots:
    void startFullScreenRecording(QScreen *screen = nullptr);
    void startRegionRecording(const QRect &region);
    void startWindowRecording(const QString &windowTitle);
    void stopRecording();

signals:
    void recordingChanged(bool recording);
    void outputPathChanged(const QString &path);
    void videoSaved(const QString &path);
    void errorOccurred(const QString &message);
    void recordingAreaChanged(const QRect &area, RecordMode mode);
    void recordingWarning(const QString &message);

private slots:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);
    void handleStartTimeout();
    void handleStopTimeout();
    void handleFinishedCheck();
    void monitorStderr();

private:
    QString resolveFfmpegPath();
    QString createOutputPath() const;
    void start(const QStringList &args);
    void startCapture(const QString &inputSpec,
                      const QStringList &extraArgs = {},
                      const QStringList &inputArgs = {});

    QProcess *m_process = nullptr;
    QTimer *m_startTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QTimer *m_stderrMonitor = nullptr;
    QString m_currentOutputPath;
    bool m_stopRequested = false;
    State m_state = State::Idle;
    bool m_errorReported = false;
    int m_frameRate = 30;
    int m_exitCode = 0;
    bool m_showCursor = true;
    int m_startTimeoutMs = 5000;
    int m_stopTimeoutMs = 5000;
    QString m_preset = QStringLiteral("fast");
    QString m_savePath;
    QString m_ffmpegPath;
    QString m_encoder;
    bool m_forceKilled = false;
    bool m_encoderDetected = false;
    std::deque<QByteArray> m_stderrChunks;
    int m_stderrSize = 0;
    QByteArray m_stderrPending;
    QStringList m_reportedWarnings;

    void detectHardwareEncoder();
};
