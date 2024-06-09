#ifndef ESTIMATORS_H
#define ESTIMATORS_H

#include <array>
#include <complex>
#include <vector>

#include <fftw3.h>
#include <sdft/sdft.h>

#include "qpmu/common.h"

namespace qpmu {

template <typename _FloatType = void>
struct FFTW
{
};

#define SPECIALIZE(type, suffix)                                        \
  template <>                                                           \
  struct FFTW<type>                                                     \
  {                                                                     \
    using Complex = fftw##suffix##_complex;                             \
    using Plan = fftw##suffix##_plan;                                   \
    static constexpr auto alloc_complex = fftw##suffix##_alloc_complex; \
    static constexpr auto malloc = fftw##suffix##_malloc;               \
    static constexpr auto plan_dft_1d = fftw##suffix##_plan_dft_1d;     \
    static constexpr auto execute = fftw##suffix##_execute;             \
    static constexpr auto destroy_plan = fftw##suffix##_destroy_plan;   \
    static constexpr auto free = fftw##suffix##_free;                   \
  };

SPECIALIZE(float, f)
SPECIALIZE(double, )
SPECIALIZE(long double, l)
#undef SPECIALIZE

enum class PhasorEstimationStrategy {
    FFT, // Fast Fourier Transform
    SDFT, // Sliding-window Discrete Fourier Transform
};

enum class FrequencyEstimationStrategy {
    PhaseDifferences, // Estimate frequency from the phase (angle) and time differences
                      // between consecutive phasors
    SamePhaseCrossings, // Estimate frequency from time differences between zero-crossings of the
                        // same phase
    ZeroCrossings, // Estimate frequency from time differences between zero-crossings of the
                   // signal samples with linear interpolation
    TimeBoundZeroCrossings, // Estimate frequency from time differences between zero-crossings of
                            // the signal samples with linear interpolation, but only within a
                            // one-second window
};

class Estimator
{
public:
    // ****** Type definitions ******

    using FloatType = qpmu::FloatType;
    using USize = qpmu::USize;
    using ISize = qpmu::ISize;
    using Complex = qpmu::Complex;
    using SdftType = sdft::SDFT<FloatType, FloatType>;
    static constexpr USize NumChannels = qpmu::NumChannels;

    struct FftwState
    {
        FFTW<FloatType>::Complex *inputs[NumChannels];
        FFTW<FloatType>::Complex *outputs[NumChannels];
        FFTW<FloatType>::Plan plans[NumChannels];
    };

    struct SdftState
    {
        std::vector<SdftType> workers;
        std::vector<std::vector<Complex>> outputs;
    };

    // ****** Constructors and destructors ******
    Estimator() : Estimator(64) { }
    Estimator(const Estimator &) = default;
    Estimator(Estimator &&) = default;
    Estimator &operator=(const Estimator &) = default;
    Estimator &operator=(Estimator &&) = default;
    ~Estimator();
    Estimator(USize window_size,
              PhasorEstimationStrategy phasor_strategy = PhasorEstimationStrategy::FFT,
              FrequencyEstimationStrategy freq_strategy =
                      FrequencyEstimationStrategy::PhaseDifferences,
              std::pair<FloatType, FloatType> voltage_params = { 1.0, 0.0 },
              std::pair<FloatType, FloatType> current_params = { 1.0, 0.0 });

    // ****** Public member functions ******
    qpmu::Estimation add_estimation(qpmu::AdcSample sample);

private:
    // ****** Private member functions ******

    // ****** Private member variables ******
    PhasorEstimationStrategy m_phasor_strategy = PhasorEstimationStrategy::FFT;
    FrequencyEstimationStrategy m_freq_strategy = FrequencyEstimationStrategy::PhaseDifferences;
    USize m_size = 0;
    FloatType m_scale_voltage = 1.0;
    FloatType m_offset_voltage = 0.0;
    FloatType m_scale_current = 1.0;
    FloatType m_offset_current = 0.0;
    std::vector<qpmu::AdcSample> m_samples = {};
    std::vector<qpmu::Estimation> m_estimations = {};
    USize m_index = 0;
    FftwState m_fftw_state = {};
    SdftState m_sdft_state = {};

    std::vector<U64> m_tbzc_xs = {};
    std::vector<U64> m_tbzc_ts = {};
    USize m_tbzc_ptr = 0;
    qpmu::U64 m_tbzc_start_micros = 0;
    qpmu::U64 m_tbzc_second_mark_micros = 0;
    qpmu::U64 m_tbzc_count_zc = 0;
};

} // namespace qpmu

#endif // ESTIMATORS_H