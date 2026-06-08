#pragma once

#include "ui/ModeSwitch.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
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

private:
    QString resolveFfmpegPath();
    QString createOutputPath() const;
    void start(const QStringList &args);
    void startCapture(const QString &inputSpec, const QStringList &extraArgs = {});

    QProcess *m_process = nullptr;
    QTimer *m_startTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QString m_currentOutputPath;
    bool m_stopRequested = false;
    int m_frameRate = 30;
    bool m_showCursor = true;
    int m_startTimeoutMs = 5000;
    int m_stopTimeoutMs = 5000;
    QString m_preset = QStringLiteral("ultrafast");
    QString m_savePath;
    QString m_ffmpegPath;
    bool m_forceKilled = false;
};
