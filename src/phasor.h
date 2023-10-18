#ifndef PHASOR_H
#define PHASOR_H

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QRandomGenerator>
#include <QString>
#include <QTimer>
#include <QProcess>
#include <QTextStream>
#include <QDebug>

#include <array>
#include <iostream>
using std::cout;

class Phasor : public QObject {
    Q_OBJECT
public:

    /// Public Types
    enum Type {
        Voltage,
        Current
    };
    struct Value {
        qreal phaseAngle;
        qreal magnitude;
        qint64 timeStamp;
    };

    enum ModeOfOperation {
        Random,
        Live
    };

    /// Public Constructors
    Phasor(QString name, QString label, Type type, QColor color, QString unit = "");

    /// Public Static Member Functions
    static void setup(
        Phasor::ModeOfOperation mode = Phasor::mode(),
        qreal maxAmplitude = Phasor::maxAmplitude(),
        qsizetype capacity = Phasor::capacity(),
        qint64 interval = Phasor::interval());
    static qreal maxAmplitude();
    static qsizetype capacity();
    static qint64 interval();
    static ModeOfOperation mode();

    /// Public Static Member Variables
    static std::array<Phasor* const, 7> phasors;

    /// Public Member Variables (constants)
    const QString name;
    const QString label;
    const Type type;
    const QColor color;
    const QString unit;

private slots:
    void addValue(const Value& value);

signals:
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
    static QProcess* m_process;
    static qint64 m_currTime;
    static QRandomGenerator m_rng;
    static QMetaObject::Connection m_connection;
};

#endif /// PHASOR_H
