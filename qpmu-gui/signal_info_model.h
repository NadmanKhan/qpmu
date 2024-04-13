#ifndef SIGNAL_INFO_MODEL_H
#define SIGNAL_INFO_MODEL_H

#include <string>

constexpr int nsignals = 6;
constexpr int nphases = 3;

enum SignalType { SignalTypeVoltage, SignalTypeCurrent };

struct SignalInfo
{
    const char *name;
    const char *colorHex;
    SignalType signalType;
    char phaseLetter;
};

static constexpr SignalInfo signalInfoList[nsignals] = {
    SignalInfo{ "VA", "#404040", SignalTypeVoltage, 'A' },
    SignalInfo{ "VB", "#ff0000", SignalTypeVoltage, 'B' },
    SignalInfo{ "VC", "#00ffff", SignalTypeVoltage, 'C' },
    SignalInfo{ "IA", "#eeee00", SignalTypeCurrent, 'A' },
    SignalInfo{ "IB", "#0000ff", SignalTypeCurrent, 'B' },
    SignalInfo{ "IC", "#00c000", SignalTypeCurrent, 'C' }
};

static constexpr int phaseIndexPairList[nphases][2] = {
    { 0, 3 } /* VA, IA */, { 1, 4 } /* VB, IB */, { 2, 5 } /* VC, IC */
};

static constexpr int signalIndex(const std::string_view &signalName)
{
    if (signalName == "VA" || signalName == "va") {
        return 0;
    }
    if (signalName == "VB" || signalName == "vb") {
        return 1;
    }
    if (signalName == "VC" || signalName == "vc") {
        return 2;
    }
    if (signalName == "IA" || signalName == "ia") {
        return 3;
    }
    if (signalName == "IB" || signalName == "ib") {
        return 4;
    }
    if (signalName == "IC" || signalName == "ic") {
        return 5;
    }
    return -1;
}

#endif // SIGNAL_INFO_MODEL_H
