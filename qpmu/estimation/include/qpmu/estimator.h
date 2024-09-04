#ifndef ESTIMATORS_H
#define ESTIMATORS_H

#include <array>
#include <complex>
#include <vector>

#include <fftw3.h>
#include <sdft/sdft.h>

#include "qpmu/defs.h"

namespace qpmu {

template <typename FloatType = double>
struct FFTW
{
};

#define SPECIALIZE_FFTW(type, suffix)                                   \
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

SPECIALIZE_FFTW(float, f)
SPECIALIZE_FFTW(double, )
SPECIALIZE_FFTW(long double, l)
#undef SPECIALIZE_FFTW

enum class PhasorEstimationStrategy {
    FFT, // Fast Fourier Transform
    SDFT, // Sliding-window Discrete Fourier Transform
};

class Estimator
{
public:
    using Float = qpmu::Float;
    using USize = qpmu::USize;
    using ISize = qpmu::ISize;
    using Complex = qpmu::Complex;
    using SdftType = sdft::SDFT<Float, Float>;
    static constexpr USize CountSignals = qpmu::SignalCount;

    struct FftwState
    {
        FFTW<Float>::Complex *inputs[CountSignals];
        FFTW<Float>::Complex *outputs[CountSignals];
        FFTW<Float>::Plan plans[CountSignals];
    };

    struct SdftState
    {
        std::vector<SdftType> workers;
        std::vector<std::vector<Complex>> outputs;
    };

    // ****** Constructors and destructors ******
    Estimator(const Estimator &) = default;
    Estimator(Estimator &&) = default;
    Estimator &operator=(const Estimator &) = default;
    Estimator &operator=(Estimator &&) = default;
    ~Estimator();
    Estimator(USize fn, USize fs,
              PhasorEstimationStrategy phasorStrategy = PhasorEstimationStrategy::FFT);

    void updateEstimation(qpmu::Sample sample);

    const qpmu::Synchrophasor &lastSynchrophasor() const;
    const qpmu::Sample &lastSample() const;
    const std::array<Float, CountSignals> &channelMagnitudes() const;

private:
    PhasorEstimationStrategy m_phasorStrategy = PhasorEstimationStrategy::FFT;
    FftwState m_fftwState = {};
    SdftState m_sdftState = {};

    std::vector<qpmu::Synchrophasor> m_syncphBuffer = {};
    USize m_syncphBufIdx = 0;

    qpmu::U64 m_windowStartTimeUs = 0;
    qpmu::U64 m_windowEndTimeUs = 0;

    std::vector<qpmu::Sample> m_sampleBuffer = {};
    USize m_sampleBufIdx = 0;
    qpmu::U64 m_zeroCrossingCount = 0;

    std::array<Float, CountSignals> m_channelMagnitudes = {};
};

} // namespace qpmu

#endif // ESTIMATORS_H