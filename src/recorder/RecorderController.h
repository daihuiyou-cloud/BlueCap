#pragma once

#include <QObject>
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
    void setStartTimeout(int ms);
    void setStopTimeout(int ms);

    static QStringList enumerateWindows();

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

private slots:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);
    void handleStartTimeout();
    void handleStopTimeout();

private:
    QString resolveFfmpegPath();
    QString createOutputPath() const;
    void start(const QStringList &args);

    QProcess *m_process = nullptr;
    QTimer *m_startTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QString m_currentOutputPath;
    bool m_stopRequested = false;
    int m_frameRate = 30;
    int m_startTimeoutMs = 5000;
    int m_stopTimeoutMs = 5000;
    QString m_preset = QStringLiteral("ultrafast");
    QString m_savePath;
    QString m_ffmpegPath;
};
