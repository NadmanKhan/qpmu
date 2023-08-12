#ifndef PHASOR_H
#define PHASOR_H

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QString>
#include <QTimer>

#include <array>
#include <deque>

class Phasor : public QObject {
    Q_OBJECT
public:
    /// Public Types
    struct Value {
        qreal phaseAngle;
        qreal magnitude;
        qint64 timeStamp;
    };
    struct Value_SOA {
        std::deque<qreal> phaseAngles;
        std::deque<qreal> magnitudes;
        std::deque<qint64> timestamps;
    };
    enum ModeOfOperation {
        Random,
        Live
    };

    /// Public Constructors
    Phasor(QString name, QString label, QColor color, QString unit = "");

    /// Public Static Member Functions
    static void setup(
        qreal maxAmplitude = Phasor::maxAmplitude(),
        qsizetype capacity = Phasor::capacity(),
        qint64 interval = Phasor::interval(),
        Phasor::ModeOfOperation mode = Phasor::mode());
    static qreal maxAmplitude();
    static qsizetype capacity();
    static qint64 interval();
    static ModeOfOperation mode();

    /// Public Member Functions
    void add(const Value& value);
    void clear();
    std::deque<Value> values() const;
    Value_SOA values_SOA() const;

    /// Public Static Member Variables
    static std::array<Phasor* const, 7> phasors;

    /// Public Member Variables (constants)
    const QString name;
    const QString label;
    const QColor color;
    const QString unit;

signals:
    /// Signals
    void newValueAdded(const Value& value);

private:
    /// Private static member functions
    static qreal random(qreal low, qreal high);

    /// Private static member variables
    static qreal m_maxAmplitude;
    static qsizetype m_capacity;
    static qint64 m_interval;
    static ModeOfOperation m_mode;
    static QTimer m_timer;
    static qint64 m_currTime;
    static QRandomGenerator m_rng;
    static QMetaObject::Connection m_connection;

    /// Private member variables
    std::deque<qreal> m_phaseAngles;
    std::deque<qreal> m_magnitudes;
    std::deque<qint64> m_timestamps;
};

#endif /// PHASOR_H
