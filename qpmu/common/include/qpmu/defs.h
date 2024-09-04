#ifndef QPMU_COMMON_DEFS_H
#define QPMU_COMMON_DEFS_H

#include <cstdint>
#include <complex>

namespace qpmu {

constexpr auto OrgName = PROJECT_ORG_NAME;
constexpr auto AppName = PROJECT_APP_NAME;
constexpr auto AppDisplayName = PROJECT_APP_NAME;
constexpr int AdcBits = PROJECT_ADC_BITS;

using Float = FLOAT_TYPE;
using Complex = std::complex<Float>;
using USize = std::size_t;
using U64 = std::uint64_t;
using U32 = std::uint16_t;
using U16 = std::uint16_t;
using ISize = ssize_t;
using I64 = std::int64_t;
using I32 = std::int16_t;
using I16 = std::int16_t;

constexpr USize SignalTypeCount = 2;
constexpr USize SignalPhaseCount = 3;
constexpr USize SignalCount = SignalPhaseCount * SignalTypeCount;

constexpr char const *SignalTypeNames[SignalTypeCount] = { "Voltage", "Current" };
constexpr char SignalTypeSymbols[SignalTypeCount] = { 'V', 'I' };
constexpr char const *SignalTypeUnitNames[SignalTypeCount] = { "Volts", "Amperes" };
constexpr char SignalTypeUnitSymbols[SignalTypeCount] = { 'V', 'A' };

constexpr char SignalPhaseNames[SignalPhaseCount] = { 'A', 'B', 'C' };

constexpr char const *SignalNames[SignalCount] = { "VA", "VB", "VC", "IA", "IB", "IC" };

enum SignalType {
    VoltageSignal = 0,
    CurrentSignal = 1,
};

enum SignalPhase {
    PhaseA = 0,
    PhaseB = 1,
    PhaseC = 2,
};

enum Signal {
    SignalVA = 0,
    SignalVB = 1,
    SignalVC = 2,
    SignalIA = 3,
    SignalIB = 4,
    SignalIC = 5,
};

constexpr SignalType SignalTypeIds[SignalCount] = { VoltageSignal, VoltageSignal, VoltageSignal,
                                                    CurrentSignal, CurrentSignal, CurrentSignal };
constexpr SignalPhase SignalPhaseIds[SignalCount] = {
    PhaseA, PhaseB, PhaseC, PhaseA, PhaseB, PhaseC
};
constexpr Signal SamePhaseSignalIds[SignalPhaseCount][SignalTypeCount] = { { SignalVA, SignalIA },
                                                                           { SignalVB, SignalIB },
                                                                           { SignalVC, SignalIC } };
constexpr Signal SameTypeSignalIds[SignalTypeCount][SignalPhaseCount] = {
    { SignalVA, SignalVB, SignalVC }, { SignalIA, SignalIB, SignalIC }
};
constexpr Signal SignalIds[SignalCount] = { SignalVA, SignalVB, SignalVC,
                                            SignalIA, SignalIB, SignalIC };

/// @brief Time-synchronized sample obtained from the sampling module.
struct Sample
{
    /// Sequence number of the sample. This is a monotonically increasing number that is used to
    /// identify the order of the samples.
    U64 seqNo;

    /// Digitized signal samples be obtained from the ADC of the sampling module. The order is as
    /// per `SignalNames`.
    U64 channels[SignalCount];

    /// Time (in microseconds) stamped by the sampling module's GPS-synchronized clock. This should
    /// be accurate to within a few microseconds of the actual time of the sample.
    U64 timestampUs;

    /// Time difference (in microseconds) between the current sample and the previous sample.
    U64 timeDeltaUs;
};

/// @brief Estimation of phasors, frequency, and rate-of-change-of-frequency, obtained from the
/// estimation module.
struct Synchrophasor
{
    /// Timestamp (in microseconds) of the source sample from which the phasor estimation was
    /// obtained.
    U64 timestampUs;

    /// Magnitudes of the phasors. This may be before or after calibration.
    Float magnitudes[SignalCount];

    /// Phase angles of the phasors (in radians)
    Float phaseAngles[SignalCount];

    /// Frequency (in Hz) of the system
    Float frequency;

    /// Rate of change of frequency (in Hz/s) of the system
    Float rocof;
};

static_assert(std::is_trivially_copyable<Sample>::value, "Sample must be trivially copyable");
static_assert(std::is_trivially_assignable<Sample, Sample>::value,
              "Sample must be trivially assignable");

static_assert(std::is_trivially_copyable<Synchrophasor>::value,
              "Synchrophasor must be trivially copyable");
static_assert(std::is_trivially_assignable<Synchrophasor, Synchrophasor>::value,
              "Synchrophasor must be trivially assignable");

} // namespace qpmu

#endif // QPMU_COMMON_DEFS_H