#ifndef QPMU_COMMON_H
#define QPMU_COMMON_H

#include <cstddef>
#include <cstdint>
#include <complex>
#include <istream>
#include <string>

namespace qpmu {

#if defined(FLOAT_TYPE)
using FloatType = FLOAT_TYPE;
#else
using FloatType = double;
#endif
using Complex = std::complex<FloatType>;

using USize = std::size_t;
using U64 = std::uint64_t;
using U32 = std::uint16_t;
using U16 = std::uint16_t;

using ISize = ssize_t;
using I64 = std::int64_t;
using I32 = std::int16_t;
using I16 = std::int16_t;

constexpr USize NumChannels = 6;
constexpr USize NumPhases = 3;
constexpr USize NumTokensAdcSample = NumChannels + 3; // seq_no, ts, delta, ch0, ch1, ..., ch5

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
                                          { "IA", "#f1dd38", SignalType::Current, SignalPhase::A },
                                          { "IB", "#0000ff", SignalType::Current, SignalPhase::B },
                                          { "IC", "#22bb45", SignalType::Current,
                                            SignalPhase::C } };

constexpr USize SignalPhasePairs[NumPhases][2] = { { 0, 3 }, { 1, 4 }, { 2, 5 } };

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

struct AdcSample
{
    U64 seq_no; // Sequence number
    U16 ch[NumChannels]; // Channel values
    U64 ts; // Timestamp (in microseconds)
    U16 delta; // Timestamp difference from previous sample (in microseconds)

    static std::string csv_header();
    friend std::string to_string(const AdcSample &sample);
    friend std::string to_csv(const AdcSample &sample);

    static AdcSample parse_string(const std::string &s);
    static AdcSample from_csv(const std::string &s);
};

struct Estimation
{
    U64 timestamp_micros; // Timestamp (in microseconds)
    FloatType phasor_mag[NumChannels]; // Phasor magnitudes
    FloatType phasor_ang[NumChannels]; // Phasor angles (in radians)
    FloatType freq; // Estimated frequency in Hz
    FloatType rocof; // Estimated rate of change of frequency in Hz/s

    static std::string csv_header();
    friend std::string to_string(const Estimation &est);
    friend std::string to_csv(const Estimation &est);
};

// utility functions

std::string phasor_to_string(const Complex &phasor);

std::string phasor_polar_to_string(const Complex &phasor);

} // namespace qpmu

#endif // QPMU_COMMON_H