#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class RecorderController : public QObject
{
    Q_OBJECT

public:
    explicit RecorderController(QObject *parent = nullptr);

    bool isRecording() const;
    QString currentOutputPath() const;

public slots:
    void startFullScreenRecording();
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

    QProcess *m_process = nullptr;
    QString m_currentOutputPath;
    bool m_stopRequested = false;
};
