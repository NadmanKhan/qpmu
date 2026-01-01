#pragma once

#include <chrono>
#include <complex>
#include <cstdint>

namespace qpmu {

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
    char const name[3];
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

using Float = float;
using Time_Point = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
using Timestamp = Time_Point::rep;
using ADC_Sample = std::uint16_t; // 12-bit ADC sample -> 16-bit unsigned integer
using Complex = std::complex<Float>;

constexpr auto Time_Resolutiion = Time_Point::period::den;

struct Reading
{
    Timestamp timestamp;
    ADC_Sample samples[N_Channels];
};

struct Estimate
{
    Complex phasors[N_Channels];
    Float frequencies[N_Channels];
    Float rocofs[N_Channels];
    Float sampling_rate;
};

struct Measurement
{
    std::size_t seq_num;
    Reading reading;
    Estimate estimate;
};

} // namespace qpmu
