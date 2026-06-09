#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <deque>

class QTimer;

class StderrMonitor : public QObject
{
    Q_OBJECT

public:
    explicit StderrMonitor(QObject *parent = nullptr);

    void feed(const QByteArray &chunk);
    void start();
    void stop();
    QByteArray drain();

signals:
    void warningDetected(const QString &message);

private slots:
    void poll();

private:
    std::deque<QByteArray> m_chunks;
    int m_totalSize = 0;
    QByteArray m_pending;
    QStringList m_reportedWarnings;
    QTimer *m_timer = nullptr;

    static constexpr int kMaxBuffer = 65536;
    static constexpr int kPollIntervalMs = 5000;
    static constexpr int kMaxPending = 4096;
    static constexpr int kTrimTo = 2048;
};
