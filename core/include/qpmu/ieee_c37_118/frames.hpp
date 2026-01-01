#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "params.hpp"

namespace qpmu {
namespace ieee_c37_118 {

template <Protocol_Version_Tag Version, typename Main_Block>
struct Frame
{
    using Main_Block_Type = Main_Block;
    static constexpr Protocol_Version_Tag Frame_Version_Tag = Version;
    static constexpr Frame_Type_Tag Frame_Type_Tag = Main_Block::Frame_Type_Tag;
    static constexpr std::size_t Frame_Size = sizeof(Frame<Version, Main_Block>);

    static_assert(Frame_Size <= 65535, "Frame size exceeds maximum allowed size of 65535 bytes.");

    /**
     * Frame synchronization word.
     * Leading byte: AA hex
     * Second byte: Frame type and Version, divided as follows:
     * Bit 7: Reserved for future definition
     * Bits 6–4: 000: Data Frame
     * 001: Header Frame
     * 010: Configuration Frame 1
     * 011: Configuration Frame 2
     * 100: Command Frame (received message)
     * Bits 3–0: Version number, in binary (1–15), version 1 for this initial
     * publication.
     */
    const std::uint16_t SYNC = Frame_Sync_Code<Frame_Type_Tag, Frame_Version_Tag>;

    /**
     * Total number of bytes in the frame, including CHK.
     * 16-bit unsigned number. Range = maximum 65535.
     */
    const std::uint16_t FRAMESIZE = Frame_Size;

    /**
     * PMU/DC ID number, 16-bit integer, assigned by user, 1 to 65 534 (0 and
     * 65 535 are reserved). Identifies device sending and receiving messages.
     */
    std::uint16_t IDCODE;

    /**
     * Time stamp, 32-bit unsigned number, SOC count starting at midnight
     * 01-Jan-1970 (UNIX time base).
     * Ranges 136 yr, rolls over 2106 AD.
     * Leap seconds are not included in count, so each year has the same number of
     * seconds except leap years, which have an extra day (86 400 s).
     */
    std::uint32_t SOC;

    /**
     * Fraction of Second and Time Quality, time of measurement for data frames
     * or time of frame transmission for non-data frames.
     * Bits 31–24: Time Quality as defined in 6.2.2.
     * Bits 23–00: Fraction-of-second, 24-bit integer number. When divided by
     * TIME_BASE yields the actual fractional second. FRACSEC used in all
     * messages to and from a given PMU shall use the same TIME_BASE that is
     * provided in the configuration message from that PMU.
     */
    std::uint32_t FRACSEC;

    //======================================================
    Main_Block main_block; // Main block of the frame
    //======================================================

    /**
     * CRC-CCITT, 16-bit unsigned integer.
     */
    std::uint16_t CHK;
};

template <typename... PMU_Stations>
struct Config_Block
{
    using PMU_Stations_Tuple = std::tuple<PMU_Stations...>;

    static constexpr Frame_Type_Tag Frame_Type_Tag = Frame_Type_Configuration_1;
    static constexpr std::uint16_t PMU_Count = sizeof...(PMU_Stations);

    /**
     * Resolution of the fractional second time stamp (FRACSEC) in all frames.
     * Bits 31–24: Reserved for flags (high 8 bits).
     * Bits 23–0: 24-bit unsigned integer that is the subdivision of the second that the
     * FRACSEC is based on.
     * The actual “fractional second of the data frame” = FRACSEC / TIME_BASE.
     */
    std::uint32_t TIME_BASE;

    /**
     * The number of PMUs included in the data frame. No limit specified. The actual
     * limit will be determined by the limit of 65 535 bytes in one frame
     * (“FRAMESIZE” field).
     */
    std::uint16_t NUM_PMU = PMU_Count;

    template <typename PMU_Station>
    struct PMU_Specific_Inner_Config_Block
    {
        using PMU_Station_Params = typename PMU_Station::PMU_Station_Params;

        /**
         * Station Name—16 bytes in ASCII format.
         */
        std::array<char, 16> STN;

        /**
         * PMU/DC ID number, 16-bit integer, defined in 6.2. Here it identifies the source of
         * data in each PMU field. If only 1 PMU is the source of the data, this ID will be the
         * same as the third field in the frame.
         */
        std::uint16_t IDCODE;

        /**
         * Data format in data frames, 16-bit flag.
         * Bits 15–4: Unused
         * Bit 3: 0 = FREQ/DFREQ 16-bit integer, 1 = floating point
         * Bit 2: 0 = analogs 16-bit integer, 1= floating point
         * Bit 1: 0 = phasors 16-bit integer, 1 = floating point
         * Bit 0: 0 = phasor real and imaginary (rectangular), 1 = magnitude and angle (polar)
         */
        const std::uint16_t FORMAT = PMU_Station_Params::Data_Format_Code;

        /**
         * Number of phasors -- 2-byte integer
         */
        const std::uint16_t PHNMR = PMU_Station_Params::Phasors_Params::Count;

        /**
         * Number of analog values -- 2-byte integer
         */
        const std::uint16_t ANNMR = PMU_Station_Params::Analogs_Params::Count;

        /**
         * Number of digital status words—2-byte integer. Digital status words are normally
         * 16-bit Boolean numbers with each bit representing a digital status channel mea-
         * sured by a PMU. A digital status word may be used in other user designated ways.
         */
        const std::uint16_t DGNMR = PMU_Station_Params::Digitals_Params::Count;

        /**
         * Phasor and channel names—16 bytes for each phasor, analog, and digital status
         * word in ASCII format in the same order as they are transmitted
         */
        std::array<std::array<char, 16>, PMU_Station_Params::Data_Channel_Count> CHNAM;

        /**
         * Conversion factor for phasor channels. Four bytes for each phasor.
         * Most significant byte: 0 = voltage; 1 = current.
         * Least significant bytes: An unsigned 24-bit word in 10–5 V or amperes per bit to
         * scale 16-bit integer data. (If transmitted data is in floating-point format, this 24-bit
         * value should be ignored.)
         */
        std::array<std::uint32_t, PMU_Station_Params::Phasors_Params::Count> PHUNIT;

        /**
         * Conversion factor for analog channels. Four bytes for each analog value.
         * Most significant byte: 0 = single point-on-wave, 1 = rms of analog input,
         * 2 = peak of analog input, 5–64 = reserved for future definition; 65–255 = user
         * definable.
         * Least significant bytes: A signed 24-bit word, user-defined scaling.
         */
        std::array<std::uint32_t, PMU_Station_Params::Analogs_Params::Count> ANUNIT;

        /**
         * Mask words for digital status words. Two 16-bit words are provided for each
         * digital word. The first will be used to indicate the normal status of the digital
         * inputs by returning a 0 when exclusive ORed (XOR) with the status word. The
         * second will indicate the current valid inputs to the PMU by having a bit set in
         * the binary position corresponding to the digital input and all other bits set to 0.
         * If digital status words are used for something other than Boolean status
         * indications, the use of masks is left to the user, such as min/max settings.
         */
        std::array<std::array<std::uint16_t, 2>, PMU_Station_Params::Digitals_Params::Count>
                DIGUNIT;

        /**
         * Nominal line frequency code (16-bit unsigned integer)
         * Bits 15–1: Reserved
         * Bit 0:
         *   1: Fundamental frequency = 50 Hz
         *   0: Fundamental frequency = 60 Hz
         */
        const std::uint16_t FNOM = PMU_Station_Params::Nominal_Frequency_Code;

        /**
         * Configuration change count is incremented each time a change is made in the
         * PMU configuration. 0 is the factory default and the initial value.
         */
        std::uint16_t CFGCNT = 0;
    };
    std::tuple<PMU_Specific_Inner_Config_Block<PMU_Stations>...> pmu_config_blocks;

    /**
     * Rate of phasor data transmissions—2-byte integer word (–32 767 to +32 767)
     * If DATA_RATE > 0, rate is number of frames per second.
     * If DATA_RATE < 0, rate is negative of seconds per frame.
     * For example: DATA_RATE = 15 is 15 frames per second and DATA_RATE =
     * –5 is 1 frame per 5 s.
     */
    std::int16_t DATA_RATE;
};

template <typename... PMU_Stations>
struct Data_Block
{
    static constexpr Frame_Type_Tag Frame_Type_Tag = Frame_Type_Data;
    static constexpr std::uint16_t PMU_Count = sizeof...(PMU_Stations);

    template <typename PMU_Station>
    struct PMU_Specific_Inner_Data_Block
    {
        using PMU_Station_Params = typename PMU_Station::PMU_Station_Params;

        /**
         * Bitmapped flags.
         * Bit 15: Data valid, 0 when PMU data is valid, 1 when invalid or PMU is in test mode.
         * Bit 14: PMU error including configuration error, 0 when no error.
         * Bit 13: PMU sync, 0 when in sync.
         * Bit 12: Data sorting, 0 by time stamp, 1 by arrival.
         * Bit 11: PMU trigger detected, 0 when no trigger.
         * Bit 10: Configuration changed, set to 1 for 1 min when configuration changed.
         * Bits 09–06: Reserved for security, presently set to 0.
         * Bits 05–04: Unlocked time:
         *   00 = sync locked, best quality
         *   01 = Unlocked for 10 s
         *   10 = Unlocked for 100 s
         *   11 = Unlocked over 1000 s
         * Bits 03–00: Trigger reason:
         *   1111–1000: Available for user definition
         *   0111: Digital 0110: Reserved
         *   0101: df/dt high 0100: Frequency high/low
         *   0011: Phase-angle diff 0010: Magnitude high
         *   0001: Magnitude low 0000: Manual
         */
        std::uint16_t STAT;

        /**
         * 16-bit integer values:
         * Rectangular format:
         *   - Real and imaginary, real value first
         *   - 16-bit signed integers, range –32 767 to +32 767
         * Polar format:
         *   - Magnitude and angle, magnitude first
         *   - Magnitude 16-bit unsigned integer range 0 to 65 535
         *   - Angle 16-bit signed integer, in radians × 104, range –31 416 to +31 416
         * 32-bit values in IEEE floating-point format:
         * Rectangular format:
         *   - Real and imaginary, in engineering units, real value first
         * Polar format:
         *   - Magnitude and angle, magnitude first and in engineering units
         *   - Angle in radians, range –π to +π
         */
        typename PMU_Station_Params::Phasors_Params::Type PHASORS;

        /**
         * Frequency deviation from nominal, in millihertz (mHz)
         * Range—nominal (50 Hz or 60 Hz) –32.767 to +32.767 Hz
         * 16-bit integer or 32-bit floating point.
         * 16-bit integer: 16-bit signed integers, range –32 767 to +32 767.
         * 32-bit floating point: actual frequency value in IEEE floating-point format.
         */
        typename PMU_Station_Params::Frequency_Params::Type FREQ;

        /**
         * Rate-of-change of frequency, in Hz per second times 100
         * Range –327.67 to +327.67 Hz per second
         * Can be 16-bit integer or IEEE floating point, same as FREQ above.
         */
        typename PMU_Station_Params::Frequency_Params::Type DFREQ;

        /**
         * Analog word. 16-bit integer. It could be sampled data such as control signal
         * or transducer value. Values and ranges defined by user.
         * Can be 16-bit integer or IEEE floating point.
         */
        typename PMU_Station_Params::Analogs_Params::Type ANALOG;

        /**
         * Digital status word. It could be bitmapped status or flag. Values and ranges
         * defined by user.
         */
        typename PMU_Station_Params::Digitals_Params::Type DIGITAL;
    };
    std::tuple<PMU_Specific_Inner_Data_Block<PMU_Stations>...> pmu_data_blocks;
};

template <std::uint16_t N = 1>
struct Header_Block
{
    static constexpr Frame_Type_Tag Frame_Type_Tag = Frame_Type_Header;

    /**
     * ASCII character array.
     */
    std::array<char, N> DATA;
};

template <std::uint16_t N = 0>
struct Command_Block
{
    static constexpr Frame_Type_Tag Frame_Type_Tag = Frame_Type_Command;

    /**
     * Command being sent to the PMU/DC.
     */
    std::uint16_t CMD;

    /**
     * Extended frame data, 16-bit words, 0 to 65 518 bytes as indicated by frame
     * size, data user defined.
     */
    std::array<std::uint16_t, N> DATA;

    static_assert(sizeof(DATA) <= 65518,
                  "Command_Block DATA size exceeds maximum allowed frame size.");
};

template <Protocol_Version_Tag Proto_Version, typename... PMU_Stations>
using Config1_Frame = Frame<Proto_Version, Config_Block<PMU_Stations...>>;

template <Protocol_Version_Tag Proto_Version, typename... PMU_Stations>
using Data_Frame = Frame<Proto_Version, Data_Block<PMU_Stations...>>;

} // namespace ieee_c37_118
} // namespace qpmu