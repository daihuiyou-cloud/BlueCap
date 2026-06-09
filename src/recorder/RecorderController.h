#pragma once

#include "RecordMode.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QByteArray>
#include <QProcess>
#include <QRect>
#include <QString>
#include <QStringList>

class QTimer;

class RecorderController : public QObject
{
    Q_OBJECT

public:
    explicit RecorderController(QObject *parent = nullptr);

    bool isRecording() const;
    QString currentOutputPath() const;
    QString currentSavePath() const;

    void setFrameRate(int fps);
    void setPreset(const QString &preset);
    void setSavePath(const QString &path);
    void setShowCursor(bool show);
    void setStartTimeout(int ms);
    void setStopTimeout(int ms);

    struct WindowEntry {
        QString displayName;
        QString title;
        qulonglong hwnd;
    };
    static QList<WindowEntry> enumerateWindows();

public slots:
    void startFullScreenRecording();
    void startRegionRecording(const QRect &region);
    void startWindowRecording(const QString &windowTitle);
    void stopRecording();

signals:
    void recordingChanged(bool recording);
    void outputPathChanged(const QString &path);
    void videoSaved(const QString &path);
    void errorOccurred(const QString &message);
    void recordingAreaChanged(const QRect &area, RecordMode mode);

private slots:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);
    void handleStartTimeout();
    void handleStopTimeout();
    void handleFinishedCheck();

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
    QString m_currentOutputPath;
    bool m_stopRequested = false;
    bool m_recording = false;
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
    QList<QByteArray> m_stderrChunks;
    int m_stderrSize = 0;

    void detectHardwareEncoderAsync();
};
