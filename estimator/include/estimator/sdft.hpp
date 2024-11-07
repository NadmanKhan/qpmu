#ifndef QPMU_PHASOR_ESTIMATOR_SDFT_H
#define QPMU_PHASOR_ESTIMATOR_SDFT_H

#include "qpmu/defs.h"

#include <cmath>
#include <complex>
#include <vector>

namespace qpmu {

template <typename SampleTypeNumeric>
class SlidingDiscreteFourierTransform
{
public:
    using SampleType = SampleTypeNumeric;

    SlidingDiscreteFourierTransform() = delete;
    SlidingDiscreteFourierTransform(const SlidingDiscreteFourierTransform &) = default;
    SlidingDiscreteFourierTransform(SlidingDiscreteFourierTransform &&) = default;
    SlidingDiscreteFourierTransform &operator=(const SlidingDiscreteFourierTransform &) = default;
    SlidingDiscreteFourierTransform &operator=(SlidingDiscreteFourierTransform &&) = default;

    SlidingDiscreteFourierTransform(size_t n)
        : m_xs(n, 0),
          m_ys(n, 0),
          m_index(0),
          m_twiddle(std::polar(Float(1), Float(-2) * std::acos(Float(-1)) / n))
    {
    }

    void addSample(SampleType x)
    {
        auto prevX = m_xs[m_index];
        m_xs[m_index] = x;
        m_ys[m_index] = m_twiddle * (x - prevX);
        m_index = (m_index + 1) % m_xs.size();
    }

    constexpr size_t size() const { return m_xs.size(); }

    constexpr size_t index() const { return m_index; }

    constexpr const Complex &twiddle() const { return m_twiddle; }

    constexpr const Complex &phasor(size_t i) const { return m_ys[(m_index + i) % m_ys.size()]; }

    constexpr const SampleType &sample(size_t i) const { return m_xs[(m_index + i) % m_xs.size()]; }

    constexpr const Complex &lastPhasor() const { return m_ys[m_index]; }

    constexpr const SampleType &lastSample() const { return m_xs[m_index]; }

private:
    std::vector<SampleType> m_xs;
    std::vector<Complex> m_ys;
    size_t m_index = 0;
    Complex m_twiddle = 1.0;
};

} // namespace qpmu

#endif // QPMU_PHASOR_ESTIMATOR_SDFT_H