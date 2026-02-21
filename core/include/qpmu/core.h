#pragma once

#include <array>
#include <chrono>
#include <complex>
#include <cstdint>
#include <type_traits>

namespace qpmu {

struct Signal_Info
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

constexpr Signal_Info Signals[N_Channels] = {
    Signal_Info{ Signal_Info::Voltage, Signal_Info::Phase_A, "VA", 'V', 'V', 'A' },
    Signal_Info{ Signal_Info::Voltage, Signal_Info::Phase_B, "VB", 'V', 'V', 'B' },
    Signal_Info{ Signal_Info::Voltage, Signal_Info::Phase_C, "VC", 'V', 'V', 'C' },
    Signal_Info{ Signal_Info::Current, Signal_Info::Phase_A, "IA", 'I', 'A', 'A' },
    Signal_Info{ Signal_Info::Current, Signal_Info::Phase_B, "IB", 'I', 'A', 'B' },
    Signal_Info{ Signal_Info::Current, Signal_Info::Phase_C, "IC", 'I', 'A', 'C' }
};

using Clock = std::chrono::system_clock;
using Timestamp = std::chrono::nanoseconds::rep;
static_assert(std::is_same<Timestamp, long long>::value, "Timestamp must be of type long long");
constexpr auto Time_Resolution = std::chrono::nanoseconds::period::den;

using Float = float;
using Complex = std::complex<Float>;
using Sample = std::uint16_t;
struct Estimate
{
    Complex phasor;
    Float frequency;
    Float rocof;
};

using Sample_Vector = std::array<Sample, N_Channels>;
using Estimate_Vector = std::array<Estimate, N_Channels>;

struct Sample_Frame
{
    std::size_t seq_num;
    Timestamp timestamp;
    Sample_Vector sample_vector;
};

struct Measurement_Frame
{
    std::size_t seq_num;
    Timestamp timestamp;
    Sample_Vector sample_vector;
    Estimate_Vector estimate_vector;
};

} // namespace qpmu
