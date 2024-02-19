#ifndef SIGNALINFOMODEL_H
#define SIGNALINFOMODEL_H

#include <QColor>
#include <QObject>
#include <QString>

class SignalInfoModel: public QObject
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

    explicit SignalInfoModel(Kind kind,
                             Phase phase,
                             QColor color,
                             QObject *parent = nullptr);
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString nameAsHtml() const;

    static QString toString(Kind kind);
    static QString toString(Phase phase);
};

constexpr qsizetype NumSignals = 6;

const std::array<SignalInfoModel, NumSignals> signalInfos = {
    SignalInfoModel(SignalInfoModel::Current, SignalInfoModel::PhaseA, QColor(0, 0, 255)),
    SignalInfoModel(SignalInfoModel::Current, SignalInfoModel::PhaseB, QColor(0, 255, 0)),
    SignalInfoModel(SignalInfoModel::Current, SignalInfoModel::PhaseC, QColor(255, 0, 0)),
    SignalInfoModel(SignalInfoModel::Voltage, SignalInfoModel::PhaseA, QColor(0, 0, 255)),
    SignalInfoModel(SignalInfoModel::Voltage, SignalInfoModel::PhaseB, QColor(0, 255, 0)),
    SignalInfoModel(SignalInfoModel::Voltage, SignalInfoModel::PhaseC, QColor(255, 0, 0)),
};

#endif //SIGNALINFOMODEL_H
