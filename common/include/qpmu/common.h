#ifndef QPMU_COMMON_H
#define QPMU_COMMON_H

#include <cstddef>
#include <cstdint>
#include <complex>

namespace qpmu {

#if defined(FLOAT_TYPE)
using FloatType = FLOAT_TYPE;
#else
using FloatType = double;
#endif
using ComplexType = std::complex<FloatType>;
using SizeType = std::size_t;
using UIntType = std::uint64_t;

constexpr SizeType NumChannels = 6;
constexpr SizeType NumPhases = 3;
constexpr SizeType NumTokensAdcSample = NumChannels + 3; // seq_no, ts, delta, ch0, ch1, ..., ch5

enum SignalType { Voltage, Current };
enum SignalPhase { A, B, C };

constexpr char SignalTypeId[2] = { 'V', 'I' };
constexpr char SignalTypeUnit[2] = { 'V', 'A' };
constexpr char SignalPhaseId[NumPhases] = { 'A', 'B', 'C' };

struct Signal
{
    char name[16];
    char colorHex[8];
    SignalType type;
    SignalPhase phase;
};

constexpr Signal Signals[NumChannels] = { { "VA", "#404040", SignalType::Voltage, SignalPhase::A },
                                          { "VB", "#ff0000", SignalType::Voltage, SignalPhase::B },
                                          { "VC", "#00ffff", SignalType::Voltage, SignalPhase::C },
                                          { "IA", "#eeee00", SignalType::Current, SignalPhase::A },
                                          { "IB", "#0000ff", SignalType::Current, SignalPhase::B },
                                          { "IC", "#00c000", SignalType::Current,
                                            SignalPhase::C } };

constexpr bool signal_is_voltage(const Signal &info)
{
    return info.type == SignalType::Voltage;
}

constexpr bool signal_is_current(const Signal &info)
{
    return info.type == SignalType::Current;
}

constexpr char signal_type_char(const Signal &info)
{
    return SignalTypeId[static_cast<int>(info.type)];
}

constexpr char signal_unit_char(const Signal &info)
{
    return SignalTypeUnit[static_cast<int>(info.type)];
}

constexpr char signal_phase_char(const Signal &info)
{
    return SignalPhaseId[static_cast<int>(info.phase)];
}

constexpr SizeType SignalPhasePairs[NumPhases][2] = { { 0, 3 }, { 1, 4 }, { 2, 5 } };

struct AdcSample
{
    UIntType seq_no; // Sequence number
    UIntType ch[NumChannels]; // Channel values
    UIntType ts; // Timestamp (in microseconds)
    UIntType delta; // Timestamp difference from previous sample (in microseconds)

    static std::string csv_header();
    friend std::string to_string(const AdcSample &sample);
    friend std::string to_csv(const AdcSample &sample);
};

struct Estimations
{
    AdcSample adc_sample; // Original sample
    ComplexType phasors[NumChannels]; // Estimated phasors
    FloatType freq; // Estimated frequency in Hz
    FloatType rocof; // Estimated rate of change of frequency in Hz/s
    FloatType power[NumChannels]; // Estimated power in Watt

    static std::string csv_header();
    friend std::string to_string(const Estimations &est);
    friend std::string to_csv(const Estimations &est);
};

// utility functions

std::string phasor_to_string(const ComplexType &phasor);
std::string phasor_polar_to_string(const ComplexType &phasor);

} // namespace qpmu

#endif // QPMU_COMMON_H