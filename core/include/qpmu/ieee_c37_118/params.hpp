#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace qpmu {
namespace ieee_c37_118 {

// Tags
// ----

enum Frame_Type_Tag : std::uint16_t {
    Frame_Type_Data = 0b000,
    Frame_Type_Header = 0b001,
    Frame_Type_Configuration_1 = 0b010,
    Frame_Type_Configuration_2 = 0b011,
    Frame_Type_Command = 0b100,
};

enum Protocol_Version_Tag : std::uint16_t {
    IEEE_C37_1118_v2005 = 1,
};

enum Command_Type_Tag : std::uint16_t {
    Command_Type_Turn_Off_Transmission = 0x0001,
    Command_Type_Turn_On_Transmission = 0x0010,
    Command_Type_Send_HDR_File = 0x0011,
    Command_Type_Send_CFG1_File = 0x0100,
    Command_Type_Send_CFG2_File = 0x0101,
    Command_Type_Extended_Frame = 0x1000,
};

enum Nominal_Frequency_Tag : std::uint16_t {
    FN_60Hz = 0,
    FN_50Hz = 1,
};

enum Number_Type_Tag : std::uint16_t {
    Fixed16 = 0,
    Float32 = 1,
};

enum Coord_Format_Tag : std::uint16_t {
    Rectangular = 0, // Real and Imaginary
    Polar = 1, // Magnitude and Angle
};

// Number Type
// -----------

template <Number_Type_Tag>
struct Number
{
    using Type = void;
};

template <>
struct Number<Fixed16>
{
    using Type = std::int16_t;
};

template <>
struct Number<Float32>
{
    static_assert(sizeof(float) == 4); // The C standard doesn't mandate it is 32 bits
    using Type = float;
};

// Frame Params
// ------------
template <Frame_Type_Tag T, Protocol_Version_Tag V = IEEE_C37_1118_v2005>
static constexpr std::uint16_t Frame_Sync_Code = (0xAA << 8) | (T << 4) | V;

// Frequency Params
// ----------------

template <Nominal_Frequency_Tag Fnom, Number_Type_Tag T = Fixed16>
struct Frequency
{
    struct Frequency_Params
    {
        using Type = typename Number<T>::Type;
        static constexpr Nominal_Frequency_Tag Nominal_Frequency_Tag = Fnom;
        static constexpr Number_Type_Tag Number_Type_Tag = T;
    };
};

// Phasors Params
// --------------

template <std::uint16_t N, Coord_Format_Tag P = Rectangular, Number_Type_Tag T = Fixed16>
struct Phasors
{
    struct Phasors_Params
    {
        using Value_Type =
                std::array<typename Number<T>::Type, 2>; // (real, imag) or (magnitude, angle);
        using Type = std::array<Value_Type, N>;
        static constexpr std::uint16_t Count = N;
        static constexpr Coord_Format_Tag Coord_Format_Tag = P;
        static constexpr Number_Type_Tag Number_Type_Tag = T;
    };
};

// Analogs Params
// --------------

template <std::uint16_t N, Number_Type_Tag T = Fixed16>
struct Analogs
{
    struct Analogs_Params
    {
        using Value_Type = typename Number<T>::Type;
        using Type = std::array<Value_Type, N>;
        static constexpr std::uint16_t Count = N;
        static constexpr Number_Type_Tag Number_Type_Tag = T;
    };
};

// Digitals Params
// ---------------

template <std::uint16_t N>
struct Digitals
{
    struct Digitals_Params
    {
        using Value_Type = std::uint16_t;
        using Type = std::array<Value_Type, N>;
        static constexpr std::uint16_t Count = N;
    };
};

// PMU Station Params
// ------------------

template <typename Frequency, typename Phasors = Phasors<0>, typename Analogs = Analogs<0>,
          typename Digitals = Digitals<0>>
struct PMU_Station
{
    struct PMU_Station_Params
    {
        using Phasors_Params = typename Phasors::Phasors_Params;
        using Analogs_Params = typename Analogs::Analogs_Params;
        using Digitals_Params = typename Digitals::Digitals_Params;
        using Frequency_Params = typename Frequency::Frequency_Params;

        static constexpr std::uint16_t Nominal_Frequency_Code =
                Frequency_Params::Nominal_Frequency_Tag;

        static constexpr std::size_t Data_Channel_Count =
                Phasors_Params::Count + Analogs_Params::Count + Digitals_Params::Count;

        static constexpr std::uint16_t Data_Format_Code = Phasors_Params::Coord_Format_Tag
                | (Phasors_Params::Number_Type_Tag << 1) | (Analogs_Params::Number_Type_Tag << 2)
                | (Frequency_Params::Number_Type_Tag << 3);
    };
};

} // namespace ieee_c37_118
} // namespace qpmu