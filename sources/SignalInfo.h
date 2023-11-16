#ifndef SIGNALINFO_H
#define SIGNALINFO_H

#include <QColor>
#include <QObject>
#include <QString>

class SignalInfo: public QObject
{
Q_OBJECT

public:

    enum Kind
    {
        Current = 0,
        Voltage = 1,
    };

    enum Phase
    {
        PhaseA = 0,
        PhaseB = 1,
        PhaseC = 2,
    };

    const Kind kind;
    const Phase phase;
    const QColor color;

    explicit SignalInfo(Kind kind,
                        Phase phase,
                        QColor color,
                        QObject *parent = nullptr);
    [[nodiscard]] QString name() const;
    QString nameAsHtml() const;

    static QString toString(Kind kind);
    static QString toString(Phase phase);
};

constexpr qsizetype NumSignals = 6;

const std::array<SignalInfo, NumSignals> signalInfos = {
    SignalInfo(SignalInfo::Current, SignalInfo::PhaseA, QColor(0, 0, 255)),
    SignalInfo(SignalInfo::Current, SignalInfo::PhaseB, QColor(0, 255, 0)),
    SignalInfo(SignalInfo::Current, SignalInfo::PhaseC, QColor(255, 0, 0)),
    SignalInfo(SignalInfo::Voltage, SignalInfo::PhaseA, QColor(0, 0, 255)),
    SignalInfo(SignalInfo::Voltage, SignalInfo::PhaseB, QColor(0, 255, 0)),
    SignalInfo(SignalInfo::Voltage, SignalInfo::PhaseC, QColor(255, 0, 0)),
};

#endif //SIGNALINFO_H
