#include "phasor.h"

qreal Phasor::m_maxAmplitude = 250.0;
qsizetype Phasor::m_capacity = 20;
qint64 Phasor::m_interval = 50;
Phasor::ModeOfOperation Phasor::m_mode = Phasor::Random;
QTimer Phasor::m_timer = QTimer();
qint64 Phasor::m_currTime = 0;

QMetaObject::Connection Phasor::m_connection;

std::array<Phasor* const, 7> Phasor::phasors = {
    new Phasor("IA", "IA", Phasor::Current, Qt::red),
    new Phasor("IB", "IB", Phasor::Current, Qt::green),
    new Phasor("IC", "IC", Phasor::Current, Qt::blue),
    new Phasor("IN", "IN", Phasor::Current, Qt::yellow),
    new Phasor("VA", "VA", Phasor::Voltage, Qt::darkRed),
    new Phasor("VB", "VB", Phasor::Voltage, Qt::darkGreen),
    new Phasor("VC", "VC", Phasor::Voltage, Qt::darkBlue)};

void Phasor::setup(
    qreal maxAmplitude,
    qsizetype capacity,
    qint64 interval,
    Phasor::ModeOfOperation mode) {

    m_maxAmplitude = maxAmplitude;
    m_capacity = capacity;
    m_interval = interval;
    m_mode = mode;
    for (auto& phasor : phasors) {
        phasor->clear();
    }
    disconnect(m_connection);
    m_connection = connect(
        &m_timer,
        &QTimer::timeout,
        [] {
            m_currTime += m_interval;
            for (auto& phasor : phasors) {
                auto magnitude = random(m_maxAmplitude * -1.0, m_maxAmplitude);
                auto phaseAngle = random(0, 1);
                phasor->add({phaseAngle, magnitude, m_currTime});
            }
        });
    m_timer.start(interval);
}

Phasor::Phasor(QString name, QString label, Phasor::Type type, QColor color, QString unit)
    : name(name), label(label), type(type), color(color), unit(unit) {}

qreal Phasor::random(qreal low, qreal high) {
    return low + (QRandomGenerator::global()->generateDouble() * (high - low));
}

qreal Phasor::maxAmplitude() {
    return m_maxAmplitude;
}

qint64 Phasor::interval() {
    return m_interval;
}

Phasor::ModeOfOperation Phasor::mode() {
    return m_mode;
}

qsizetype Phasor::capacity() {
    return m_capacity;
}

void Phasor::add(const Value& value) {
    m_phaseAngles.push_back(value.phaseAngle);
    m_magnitudes.push_back(value.magnitude);
    m_timestamps.push_back(value.timeStamp);

    if ((qsizetype)m_phaseAngles.size() > m_capacity) {
        m_phaseAngles.pop_front();
        m_magnitudes.pop_front();
        m_timestamps.pop_front();
    }

    emit newValueAdded(value);
}

void Phasor::clear() {
    m_phaseAngles.clear();
    m_magnitudes.clear();
    m_timestamps.clear();
}

Phasor::ListType<Phasor::Value> Phasor::values() const {
    Phasor::ListType<Phasor::Value> list;
    auto it_phaseAngle = m_phaseAngles.begin();
    auto it_magnitude = m_magnitudes.begin();
    auto it_timestamp = m_timestamps.begin();

    while (it_phaseAngle != m_phaseAngles.end()) {
        list.push_back(Phasor::Value{*it_phaseAngle, *it_magnitude, *it_timestamp});

        ++it_phaseAngle;
        ++it_magnitude;
        ++it_timestamp;
    }

    return list;
}

Phasor::Value_SOA Phasor::values_SOA() const {
    Phasor::ListType<qreal> phaseAngles(m_phaseAngles.begin(), m_phaseAngles.end());
    Phasor::ListType<qreal> magnitudes(m_magnitudes.begin(), m_magnitudes.end());
    Phasor::ListType<qint64> timestamps(m_timestamps.begin(), m_timestamps.end());
    return {phaseAngles, magnitudes, timestamps};
}
