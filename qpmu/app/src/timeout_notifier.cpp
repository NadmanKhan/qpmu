#include "timeout_notifier.h"

TimeoutNotifier::TimeoutNotifier(QObject *parent) : QObject(parent) { }

TimeoutNotifier::TimeoutNotifier(QTimer *timer, QObject *parent) : QObject(parent), m_timer(timer)
{
    connect(m_timer, &QTimer::timeout, this, &TimeoutNotifier::update);
    m_running = true;
}

TimeoutNotifier::TimeoutNotifier(QTimer *timer, quint32 targetIntervalMs, QObject *parent)
    : QObject(parent), m_timer(timer), m_target(targetIntervalMs)
{
    auto intervalMs = m_timer->interval();
    m_target = (targetIntervalMs + intervalMs - 1) / intervalMs;
    connect(m_timer, &QTimer::timeout, this, &TimeoutNotifier::update);
    m_running = true;
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
    if (!m_running)
        return;
    ++m_counter;
    m_counter *= bool(m_counter < m_target);
    if (m_counter == 0)
        emit timeout();
}