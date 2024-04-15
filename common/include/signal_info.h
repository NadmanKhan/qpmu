#ifndef SIGNAL_INFO_MODEL_H
#define SIGNAL_INFO_MODEL_H

#include <string>
#include <string_view>

constexpr size_t NUM_SIGNALS = 6;
constexpr size_t NUM_PHASES = 3;

enum SignalType { SignalTypeVoltage, SignalTypeCurrent };

struct SignalInfo
{
    const char *name;
    const char *colorHex;
    SignalType signalType;
    char phaseLetter;
};

static constexpr SignalInfo signalInfoList[NUM_SIGNALS] = {
    SignalInfo{ "VA", "#404040", SignalTypeVoltage, 'A' },
    SignalInfo{ "VB", "#ff0000", SignalTypeVoltage, 'B' },
    SignalInfo{ "VC", "#00ffff", SignalTypeVoltage, 'C' },
    SignalInfo{ "IA", "#eeee00", SignalTypeCurrent, 'A' },
    SignalInfo{ "IB", "#0000ff", SignalTypeCurrent, 'B' },
    SignalInfo{ "IC", "#00c000", SignalTypeCurrent, 'C' }
};

static constexpr int phaseIndexPairList[NUM_PHASES][2] = {
    { 0, 3 } /* VA, IA */, { 1, 4 } /* VB, IB */, { 2, 5 } /* VC, IC */
};

static constexpr int index(const std::string_view &name)
{
    if (name == "VA" || name == "va") {
        return 0;
    }
    if (name == "VB" || name == "vb") {
        return 1;
    }
    if (name == "VC" || name == "vc") {
        return 2;
    }
    if (name == "IA" || name == "ia") {
        return 3;
    }
    if (name == "IB" || name == "ib") {
        return 4;
    }
    if (name == "IC" || name == "ic") {
        return 5;
    }
    return -1;
}

#endif // SIGNAL_INFO_MODEL_H
