#ifndef PHASOR_ESTIMATOR_H
#define PHASOR_ESTIMATOR_H

#include <vector>

#include <fftw3.h>

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

class PhasorEstimator
{
public:
    // ****** Constructors and destructors ******
    PhasorEstimator(const PhasorEstimator &) = default;
    PhasorEstimator(PhasorEstimator &&) = default;
    PhasorEstimator &operator=(const PhasorEstimator &) = default;
    PhasorEstimator &operator=(PhasorEstimator &&) = default;
    ~PhasorEstimator();
    PhasorEstimator(size_t fn, size_t fs);

    void updateEstimation(const qpmu::Sample &sample);

    const qpmu::Estimation &currentEstimation() const;
    const qpmu::Sample &currentSample() const;

private:
    struct
    {
        FFTW<Float>::Complex *inputs[CountSignals];
        FFTW<Float>::Complex *outputs[CountSignals];
        FFTW<Float>::Plan plans[CountSignals];
    } m_fftw = {};

    std::vector<qpmu::Estimation> m_estimationBuffer = {};
    std::vector<qpmu::Sample> m_sampleBuffer = {};
    size_t m_estimationBufIdx = 0;
    size_t m_sampleBufIdx = 0;

    int64_t m_windowStartTime = 0;
    int64_t m_windowEndTime = 0;
};

} // namespace qpmu

#endif // PHASOR_ESTIMATOR_H