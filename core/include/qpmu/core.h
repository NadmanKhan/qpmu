#pragma once

#include <array>
#include <chrono>
#include <complex>
#include <cstdint>

namespace qpmu {

struct Signal_Info
{
    enum Type_ID {
        Type_Voltage = 0,
        Type_Current = 1,
    };

    enum Phase_ID {
        Phase_A = 0,
        Phase_B = 1,
        Phase_C = 2,
    };

    Type_ID type_id;
    Phase_ID phase_id;
    char type_symbol;
    char phase_symbol;
    char unit_symbol;
    char name[3];
};

constexpr std::array<Signal_Info, 6> Signal_Infos = {
    Signal_Info{ Signal_Info::Type_Voltage, Signal_Info::Phase_A, 'V', 'A', 'V', "VA" },
    Signal_Info{ Signal_Info::Type_Voltage, Signal_Info::Phase_B, 'V', 'B', 'V', "VB" },
    Signal_Info{ Signal_Info::Type_Voltage, Signal_Info::Phase_C, 'V', 'C', 'V', "VC" },
    Signal_Info{ Signal_Info::Type_Current, Signal_Info::Phase_A, 'I', 'A', 'A', "IA" },
    Signal_Info{ Signal_Info::Type_Current, Signal_Info::Phase_B, 'I', 'B', 'A', "IB" },
    Signal_Info{ Signal_Info::Type_Current, Signal_Info::Phase_C, 'I', 'C', 'A', "IC" }
};

using Timestamp = std::chrono::nanoseconds::rep;
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

using Sample_Vector = std::array<Sample, Signal_Infos.size()>;
using Estimate_Vector = std::array<Estimate, Signal_Infos.size()>;

struct Sample_Frame
{
    std::size_t seq_num;
    Timestamp timestamp;
    Sample_Vector sample_vector;
};

struct Measurement_Frame
{
    Sample_Frame sample_frame;
    Estimate_Vector estimate_vector;
};

} // namespace qpmu
