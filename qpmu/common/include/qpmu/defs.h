#ifndef QPMU_COMMON_DEFS_H
#define QPMU_COMMON_DEFS_H

#include <array>
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

constexpr USize CountSignalTypes = 2;
constexpr USize CountSignalPhases = 3;
constexpr USize CountSignals = CountSignalPhases * CountSignalTypes;

constexpr char const *NameOfSignalType[CountSignalTypes] = { "Voltage", "Current" };
constexpr char const *SymbolOfSignalType[CountSignalTypes] = { "V", "I" };
constexpr char const *UnitNameOfSignalType[CountSignalTypes] = { "Volts", "Amperes" };
constexpr char const *UnitSymbolOfSignalType[CountSignalTypes] = { "V", "A" };

constexpr char const *NameOfSignalPhase[CountSignalPhases] = { "A", "B", "C" };

constexpr char const *NameOfSignal[CountSignals] = { "VA", "VB", "VC", "IA", "IB", "IC" };

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

constexpr SignalType TypeOfSignal[CountSignals] = { VoltageSignal, VoltageSignal, VoltageSignal,
                                                    CurrentSignal, CurrentSignal, CurrentSignal };
constexpr SignalPhase PhaseOfSignal[CountSignals] = {
    PhaseA, PhaseB, PhaseC, PhaseA, PhaseB, PhaseC
};
constexpr Signal SignalsOfPhase[CountSignalPhases][CountSignalTypes] = { { SignalVA, SignalIA },
                                                                         { SignalVB, SignalIB },
                                                                         { SignalVC, SignalIC } };
constexpr Signal SignalsOfType[CountSignalTypes][CountSignalPhases] = {
    { SignalVA, SignalVB, SignalVC }, { SignalIA, SignalIB, SignalIC }
};
constexpr Signal SignalId[CountSignals] = { SignalVA, SignalVB, SignalVC,
                                            SignalIA, SignalIB, SignalIC };

struct RawSample
{
    uint64_t sampleNo = {}; // Sample number within the batch
    uint16_t data[CountSignals] = {}; // Data from a single scan
};

constexpr USize CountRawSamplesPerBatch = 128;

struct RawSampleBatch
{
    uint64_t batchNo = {}; // Batch number
    uint64_t firstSampleTimeUs = {}; // Timestamp at the start of the batch
    uint64_t lastSampleTimeUs = {}; // Timestamp at the end of the batch
    uint64_t timeSinceLastBatchUs = {}; // Time since the last batch
    RawSample samples[CountRawSamplesPerBatch] = {}; // Buffer for 128 samples
};

/// @brief Time-stamped ADC sample vector obtained from the sampling module.
struct Sample
{
    /// Sequence number of the sample. This is a monotonically increasing number that is used to
    /// identify the order of the samples.
    U64 seqNo = {};

    /// Time (in microseconds) stamped by the sampling module's GPS-synchronized clock. This should
    /// be accurate to within a few microseconds of the actual time of the sample.
    U64 timestampUs = {};

    /// Time (in microseconds) since the last sample.
    U64 timeDeltaUs = {};

    /// Digitized signal samples be obtained from the ADC of the sampling module. The order is as
    /// per `SignalNames`.
    U64 channels[CountSignals] = {};
};

using SampleFieldVector =
        std::array<U64, 3 + CountSignals>; // seq, timestampUs, timeDeltaUs, channels

/// @brief Estimations of phasors, frequency and rate-of-change-of-frequency
/// of each phasor, and sapling rate, to be computed by the estimations module
/// from sequence of samples.
struct Estimation
{
    /// Raw/uncalibrated phasors
    Complex phasors[CountSignals] = {};

    /// Frequency of the phasors (in Hz)
    Float frequencies[CountSignals] = {};

    /// Rate of change of frequency (ROCOF) of the phasors (in Hz/s)
    Float rocofs[CountSignals] = {};

    /// Sampling rate of the samples (in Hz)
    Float samplingRate = {};
};

static_assert(std::is_trivially_copyable<Sample>::value, "Sample must be trivially copyable");
static_assert(std::is_trivially_assignable<Sample, Sample>::value,
              "Sample must be trivially assignable");

static_assert(std::is_trivially_copyable<Estimation>::value,
              "Synchrophasor must be trivially copyable");
static_assert(std::is_trivially_assignable<Estimation, Estimation>::value,
              "Synchrophasor must be trivially assignable");

} // namespace qpmu

#endif // QPMU_COMMON_DEFS_H