#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

class QTimer;

class RecordingSession : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Starting, Recording, Stopping };

    explicit RecordingSession(QObject *parent = nullptr);

    State state() const;
    bool isRecording() const;
    int exitCode() const;
    bool wasForceKilled() const;
    QProcess *process() const;
    qint64 writeStdin(const QByteArray &data);

public slots:
    void start(const QString &program, const QStringList &args, int timeoutMs);
    void stop(int timeoutMs);

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus status);
    void processError(QProcess::ProcessError error);
    void startTimeout();
    void stopTimeout();
    void stateChanged(RecordingSession::State state);

private slots:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);
    void handleStartTimeout();
    void handleStopTimeout();

private:
    void setState(State s);

    QProcess *m_process = nullptr;
    QTimer *m_startTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    State m_state = State::Idle;
    int m_exitCode = 0;
    bool m_stopRequested = false;
    bool m_forceKilled = false;
};
