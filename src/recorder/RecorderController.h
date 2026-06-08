#pragma once

#include <QObject>
#include <QProcess>
#include <QRect>
#include <QString>
#include <QStringList>

class RecorderController : public QObject
{
    Q_OBJECT

public:
    explicit RecorderController(QObject *parent = nullptr);

    bool isRecording() const;
    QString currentOutputPath() const;

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
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);

private:
    QString resolveFfmpegPath() const;
    QString createOutputPath() const;
    void start(const QStringList &args);

    QProcess *m_process = nullptr;
    QString m_currentOutputPath;
    bool m_stopRequested = false;
};
