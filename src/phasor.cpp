#include "phasor.h"

qreal Phasor::m_maxAmplitude = 20.0;
qsizetype Phasor::m_capacity = 20;
qint64 Phasor::m_interval = 50;
Phasor::ModeOfOperation Phasor::m_mode = Phasor::Random;
QTimer Phasor::m_timer = QTimer();
QProcess* Phasor::m_process = nullptr;
qint64 Phasor::m_currTime = 0;

QMetaObject::Connection Phasor::m_connection;

QPair<QString, QString> getKeyValuePair(const QString& str) {
    auto tokens = str.split('=');
    return {tokens[0].trimmed(), tokens[1].trimmed()};
}

std::array<Phasor* const, 7> Phasor::phasors = {
    new Phasor("IA", "IA", Phasor::Current, Qt::yellow),
    new Phasor("IB", "IB", Phasor::Current, Qt::blue),
    new Phasor("IC", "IC", Phasor::Current, Qt::green),
    new Phasor("VA", "VA", Phasor::Voltage, Qt::gray),
    new Phasor("VB", "VB", Phasor::Voltage, Qt::red),
    new Phasor("VC", "VC", Phasor::Voltage, Qt::cyan),
    new Phasor("IN", "IN", Phasor::Current, Qt::magenta)};

void Phasor::setup(
    Phasor::ModeOfOperation mode,
    qreal maxAmplitude,
    qsizetype capacity,
    qint64 interval) {

    m_maxAmplitude = maxAmplitude;
    m_capacity = capacity;
    m_interval = interval;
    m_mode = mode;

    disconnect(m_connection);

    if (m_mode == Phasor::Random) {
        m_connection = connect(
            &m_timer,
            &QTimer::timeout,
            [] {
                m_currTime += m_interval;
                for (auto& phasor : phasors) {
                    auto magnitude = random(m_maxAmplitude * -1.0, m_maxAmplitude);
                    auto phaseAngle = random(0, 1);
                    auto value = Phasor::Value{phaseAngle, magnitude, m_currTime};
                    phasor->addValue(value);
                }
            });

        m_timer.start(interval);

    } else if (m_mode == Phasor::Live) {
        delete m_process;
        m_process = new QProcess();
        m_process->start("/usr/bin/python3",
                         QStringList() <<
                             "/home/nadman/Documents/pmu-gui/src/adc_simulator.py");


        char buf[128];

        m_connection = connect(
            m_process,
            &QProcess::readyReadStandardOutput,
            [&buf] {
                while (m_process->readLine(buf, sizeof(buf))) {
                    auto line = QString(buf);
                    auto kvPairs = line.split(',');
                    Q_ASSERT(kvPairs.size() == 8);
                    auto [tsK, tsV] = getKeyValuePair(kvPairs[6]);
                    auto timeStamp = tsV.toLongLong();
                    for (int i = 0; i < 6; ++i) {
                        auto [kStr, vStr] = getKeyValuePair(kvPairs[i]);
                        auto magnitude = vStr.toFloat();
                        auto phaseAngle = random(0, 1);
                        auto value = Phasor::Value{phaseAngle, magnitude, timeStamp};
                        Phasor::phasors[i]->addValue(value);
                    }

                }
            }
        );
    }
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

void Phasor::addValue(const Value &value) {
    emit newValueAdded(value);
}

qsizetype Phasor::capacity() {
    return m_capacity;
}
