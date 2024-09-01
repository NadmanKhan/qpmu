#include "timeout_notifier.h"

TimeoutNotifier::TimeoutNotifier(QTimer *timer, QObject *parent)
    : TimeoutNotifier(timer, timer->interval(), parent)
{
}

TimeoutNotifier::TimeoutNotifier(QTimer *timer, quint32 targetIntervalMs, QObject *parent)
    : QObject(parent), m_timer(timer), m_targetIntervalMs(targetIntervalMs)
{
    auto intervalMs = m_timer->interval();
    m_targetCount = (targetIntervalMs + intervalMs - 1) / intervalMs;
    connect(m_timer, &QTimer::timeout, this, &TimeoutNotifier::update);
}

bool TimeoutNotifier::isTimeout() const
{
    return m_running && m_counter == 0;
}

void TimeoutNotifier::start()
{
    m_counter = 0;
    m_running = true;
}

void TimeoutNotifier::stop()
{
    m_running = false;
}

void TimeoutNotifier::update()
{
    if (!m_running) {
        return;
    }
    {
        /// counter = (counter + 1) % targetCount
        ++m_counter;
        m_counter *= bool(m_counter < m_targetCount);
    }
    if (m_counter == 0) {
        emit timeout();
    }
}