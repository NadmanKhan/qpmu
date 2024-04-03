#ifndef SIGNAL_LIST_MODEL_H
#define SIGNAL_LIST_MODEL_H

#include <QList>
#include <QString>
#include <utility>

constexpr int nsignals = 6;

enum SignalType { SignalTypeVoltage, SignalTypeCurrent };

struct SignalInfoModel
{
    const char name[3];
    const char colorHex[8];
    SignalType signalType;
};

constexpr std::array<SignalInfoModel, nsignals> listSignalInfoModel = {
    SignalInfoModel{ "VA", "#000000", SignalTypeVoltage },
    SignalInfoModel{ "VB", "#ff0000", SignalTypeVoltage },
    SignalInfoModel{ "VC", "#00ffff", SignalTypeVoltage },
    SignalInfoModel{ "IA", "#ffdd00", SignalTypeCurrent },
    SignalInfoModel{ "IB", "#0000ff", SignalTypeCurrent },
    SignalInfoModel{ "IC", "#00ff88", SignalTypeCurrent }
};
#endif // SIGNAL_LIST_MODEL_H
