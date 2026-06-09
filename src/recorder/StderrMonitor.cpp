#include "StderrMonitor.h"

#include <QTimer>

StderrMonitor::StderrMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(kPollIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &StderrMonitor::poll);
}

void StderrMonitor::feed(const QByteArray &chunk)
{
    m_totalSize += chunk.size();
    m_chunks.push_back(chunk);
    while (m_totalSize > kMaxBuffer && !m_chunks.empty()) {
        m_totalSize -= m_chunks.front().size();
        m_chunks.pop_front();
    }

    m_pending.append(chunk);
    if (m_pending.size() > kMaxPending)
        m_pending = m_pending.right(kTrimTo);
}

void StderrMonitor::start()
{
    m_pending.clear();
    m_reportedWarnings.clear();
    m_timer->start();
}

void StderrMonitor::stop()
{
    m_timer->stop();
}

QByteArray StderrMonitor::drain()
{
    QByteArray result;
    result.reserve(m_totalSize);
    for (const auto &chunk : std::as_const(m_chunks))
        result.append(chunk);
    m_chunks.clear();
    m_totalSize = 0;
    return result;
}

void StderrMonitor::poll()
{
    if (m_pending.isEmpty())
        return;

    const QString text = QString::fromUtf8(m_pending);
    m_pending.clear();

    struct WarningPattern {
        QLatin1String pattern;
        QString message;
    };
    static const WarningPattern patterns[] = {
        { QLatin1String("dup="),   QStringLiteral("检测到重复/丢帧，系统负载可能过高") },
        { QLatin1String("drop="),  QStringLiteral("检测到丢帧，系统负载可能过高") },
        { QLatin1String("speed="), QStringLiteral("编码速度低于实时，建议降低帧率或画质") },
        { QLatin1String("too many packets buffered"), QStringLiteral("编码器缓冲积压，系统负载过高") },
        { QLatin1String("Invalid data found"),        QStringLiteral("屏幕数据异常，请检查是否有其他录屏程序") },
    };

    for (const auto &wp : patterns) {
        if (text.contains(wp.pattern) && !m_reportedWarnings.contains(wp.message)) {
            m_reportedWarnings.append(wp.message);
            emit warningDetected(wp.message);
        }
    }
}
