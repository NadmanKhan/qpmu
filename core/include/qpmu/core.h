#pragma once

#include <chrono>
#include <complex>
#include <cstdint>

namespace qpmu {

#if defined(USE_DOUBLE) && USE_DOUBLE
using Float = double;
#else
using Float = float;
#endif

using Complex = std::complex<Float>;

// Time point in nanoseconds resolution using system clock (because we need the wall/NTP time)
using Time_Point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

constexpr auto One_Second_Period = Time_Point::duration::period::den;

struct SignalInfo
{
    enum Type {
        Voltage = 0,
        Current = 1,
    };

    enum Phase {
        Phase_A = 0,
        Phase_B = 1,
        Phase_C = 2,
    };

    Type type;
    Phase phase;
    char const id[3];
    char type_symbol;
    char unit_symbol;
    char phase_symbol;
};

constexpr std::uint64_t N_Signal_Types = 2;
constexpr std::uint64_t N_Signal_Phases = 3;
constexpr std::uint64_t N_Channels = N_Signal_Phases * N_Signal_Types;

constexpr SignalInfo Signals[N_Channels] = {
    SignalInfo{ SignalInfo::Voltage, SignalInfo::Phase_A, "VA", 'V', 'V', 'A' },
    SignalInfo{ SignalInfo::Voltage, SignalInfo::Phase_B, "VB", 'V', 'V', 'B' },
    SignalInfo{ SignalInfo::Voltage, SignalInfo::Phase_C, "VC", 'V', 'V', 'C' },
    SignalInfo{ SignalInfo::Current, SignalInfo::Phase_A, "IA", 'I', 'A', 'A' },
    SignalInfo{ SignalInfo::Current, SignalInfo::Phase_B, "IB", 'I', 'A', 'B' },
    SignalInfo{ SignalInfo::Current, SignalInfo::Phase_C, "IC", 'I', 'A', 'C' }
};

using Sample_Value = std::uint16_t;
using Timestamp = Time_Point::duration::rep;

struct Sample
{
    Timestamp timestamp;
    Sample_Value values[N_Channels];
};

struct Estimation
{
    Complex phasors[N_Channels];
    Float frequencies[N_Channels];
    Float rocofs[N_Channels];
    Float sampling_rate;
};

} // namespace qpmu
