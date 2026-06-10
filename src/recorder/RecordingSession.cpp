#include "RecordingSession.h"

#include <QTimer>

RecordingSession::RecordingSession(QObject *parent)
    : QObject(parent)
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::started, this, &RecordingSession::handleStarted);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &RecordingSession::handleFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &RecordingSession::handleProcessError);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &RecordingSession::stderrReady);

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    connect(m_startTimer, &QTimer::timeout, this, &RecordingSession::handleStartTimeout);

    m_stopTimer = new QTimer(this);
    m_stopTimer->setSingleShot(true);
    connect(m_stopTimer, &QTimer::timeout, this, &RecordingSession::handleStopTimeout);
}

RecordingSession::State RecordingSession::state() const
{
    return m_state;
}

bool RecordingSession::isRecording() const
{
    return m_state == State::Recording;
}

int RecordingSession::exitCode() const
{
    return m_exitCode;
}

bool RecordingSession::wasForceKilled() const
{
    return m_forceKilled;
}

QByteArray RecordingSession::readStderr()
{
    return m_process->readAllStandardError();
}

qint64 RecordingSession::writeStdin(const QByteArray &data)
{
    return m_process->write(data);
}

void RecordingSession::start(const QString &program, const QStringList &args, int timeoutMs)
{
    if (m_state != State::Idle) return;

    setState(State::Starting);
    m_stopRequested = false;
    m_forceKilled = false;

    m_process->start(program, args);
    m_startTimer->start(timeoutMs);
}

void RecordingSession::stop(int timeoutMs)
{
    if (m_state != State::Recording) return;

    setState(State::Stopping);
    m_stopRequested = true;
    m_stopTimer->start(timeoutMs);
}

void RecordingSession::setState(State s)
{
    if (m_state != s) {
        m_state = s;
        emit stateChanged(m_state);
    }
}

void RecordingSession::handleStarted()
{
    m_startTimer->stop();
    setState(State::Recording);
    emit started();
}

void RecordingSession::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    m_startTimer->stop();
    m_stopTimer->stop();
    m_exitCode = exitCode;
    setState(State::Idle);
    emit finished(exitCode, status);
}

void RecordingSession::handleProcessError(QProcess::ProcessError error)
{
    m_startTimer->stop();
    emit processError(error);
}

void RecordingSession::handleStartTimeout()
{
    if (m_state != State::Starting) return;
    setState(State::Idle);
    m_process->kill();
    emit startTimeout();
}

void RecordingSession::handleStopTimeout()
{
    if (m_process->state() == QProcess::NotRunning)
        return;

    m_process->terminate();

    // Non-blocking chain: wait 3s → kill → wait 2s → force idle
    auto *killTimer = new QTimer(this);
    killTimer->setSingleShot(true);
    connect(killTimer, &QTimer::timeout, this, [this, killTimer]() {
        killTimer->deleteLater();
        if (m_process->state() == QProcess::NotRunning)
            return;

        m_forceKilled = true;
        m_process->kill();

        auto *forceKillTimer = new QTimer(this);
        forceKillTimer->setSingleShot(true);
        connect(forceKillTimer, &QTimer::timeout, this, [this, forceKillTimer]() {
            forceKillTimer->deleteLater();
            if (m_process->state() != QProcess::NotRunning) {
                setState(State::Idle);
                emit stopTimeout();
            }
        });
        forceKillTimer->start(2000);
    });
    killTimer->start(3000);
}
