#include "qpmu/estimator.h"
#include "qpmu/defs.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>

using namespace qpmu;

template <class Numeric>
constexpr bool isPositive(Numeric x)
{
    if constexpr (std::is_integral<Numeric>::value) {
        return x > 0;
    } else {
        return x > 0 + std::numeric_limits<Numeric>::epsilon();
    }
}

template <class IntValue, class IntTime>
constexpr IntTime zeroCrossingTime(IntTime t0, IntValue x0, IntTime t1, IntValue x1)
{
    return t0 + (0 - x0) * (t1 - t0) / (x1 - x0);
}

PhasorEstimator::~PhasorEstimator()
{
    for (size_t i = 0; i < CountSignals; ++i) {
        FFTW<Float>::destroy_plan(m_fftw.plans[i]);
        FFTW<Float>::free(m_fftw.inputs[i]);
        FFTW<Float>::free(m_fftw.outputs[i]);
    }
}

PhasorEstimator::PhasorEstimator(size_t fn, size_t fs)
{
    assert(fn > 0);
    assert(fs % fn == 0);

    m_estimationBuffer.resize(fs
                              / fn); // hold one full cycle (fs / fn = number of samples per cycle)
    m_sampleBuffer.resize(5 * fs); // hold at least 1 second of samples

    for (size_t i = 0; i < CountSignals; ++i) {
        m_fftw.inputs[i] = FFTW<Float>::alloc_complex(m_estimationBuffer.size());
        m_fftw.outputs[i] = FFTW<Float>::alloc_complex(m_estimationBuffer.size());
        m_fftw.plans[i] = FFTW<Float>::plan_dft_1d(m_estimationBuffer.size(), m_fftw.inputs[i],
                                                   m_fftw.outputs[i], FFTW_FORWARD, FFTW_ESTIMATE);
        for (size_t j = 0; j < m_estimationBuffer.size(); ++j) {
            m_fftw.inputs[i][j][0] = 0;
            m_fftw.inputs[i][j][1] = 0;
        }
    }
}

#define NEXT(i, x) (((i) != (m_##x##Buffer.size() - 1)) * ((i) + 1))
#define PREV(i, x) (((i) != 0) * ((i)-1) + ((i) == 0) * (m_##x##Buffer.size() - 1))

#define ESTIMATION_NEXT(i) NEXT(i, estimation)
#define ESTIMATION_PREV(i) PREV(i, estimation)

#define SAMPLE_NEXT(i) NEXT(i, sample)
#define SAMPLE_PREV(i) PREV(i, sample)

const Estimation &PhasorEstimator::currentEstimation() const
{
    return m_estimationBuffer[m_estimationBufIdx];
}

const Sample &PhasorEstimator::currentSample() const
{
    return m_sampleBuffer[m_sampleBufIdx];
}

void PhasorEstimator::updateEstimation(const Sample &sample)
{
    m_sampleBuffer[m_sampleBufIdx] = sample;

    const Estimation &prevEstimation = m_estimationBuffer[ESTIMATION_PREV(m_estimationBufIdx)];
    Estimation &currEstimation = m_estimationBuffer[m_estimationBufIdx];

    const Sample &prevSample = m_sampleBuffer[SAMPLE_PREV(m_sampleBufIdx)];
    const Sample &currSample = m_sampleBuffer[m_sampleBufIdx];

    { /// Estimate phasors
        for (size_t ch = 0; ch < CountSignals; ++ch) {

            /// Shift the previous inputs
            for (size_t j = 1; j < m_estimationBuffer.size(); ++j) {
                m_fftw.inputs[ch][j - 1][0] = m_fftw.inputs[ch][j][0];
                m_fftw.inputs[ch][j - 1][1] = m_fftw.inputs[ch][j][1];
            }

            /// Add the new sample's data
            m_fftw.inputs[ch][m_estimationBuffer.size() - 1][0] = sample.channels[ch];
            m_fftw.inputs[ch][m_estimationBuffer.size() - 1][1] = 0;

            /// Execute the FFT plan
            FFTW<Float>::execute(m_fftw.plans[ch]);

            /// Phasor = output corresponding to the fundamental frequency
            Complex phasor = { m_fftw.outputs[ch][1][0], m_fftw.outputs[ch][1][1] };
            phasor /= Float(m_estimationBuffer.size());
            currEstimation.phasors[ch] = phasor;
        }
    }
    { /// Estimate frequency and ROCOF, and sampling rate

        if (m_windowStartTime == 0) {
            m_windowStartTime = sample.timestampUsec;
            m_windowEndTime = m_windowStartTime + TimeDenom;
        }

        std::copy(prevEstimation.frequencies, prevEstimation.frequencies + CountSignals,
                  currEstimation.frequencies);

        std::copy(prevEstimation.rocofs, prevEstimation.rocofs + CountSignals,
                  currEstimation.rocofs);

        currEstimation.samplingRate = prevEstimation.samplingRate;

        if (m_windowEndTime <= sample.timestampUsec) {
            /// The 1-second window has ended, hence
            /// - estimate
            ///   * channel frequencies,
            ///   * channel ROCOFs, and
            ///   * sampling rate
            /// - reset the window variables

            for (size_t ch = 0; ch < CountSignals; ++ch) {
                { /// Frequency estimation

                    /// Get the "zero" value -- the value that is halfway between the maximum and
                    /// minimum values in the window
                    uint64_t maxValue = 0;
                    for (size_t i = 0; i <= m_sampleBufIdx; ++i) {
                        if (m_sampleBuffer[i].channels[ch] > maxValue) {
                            maxValue = m_sampleBuffer[i].channels[ch];
                        }
                    }
                    const int64_t zeroValue = maxValue / 2;

                    /// Get the zero crrossing count and the first and last zero crossing times
                    uint64_t countZeroCrossings = 0;
                    uint64_t firstCrossingTime = std::numeric_limits<uint64_t>::max();
                    uint64_t lastCrossingTime = std::numeric_limits<uint64_t>::min();
                    for (size_t i = 1; i <= m_sampleBufIdx; ++i) {
                        const int64_t &x0 = (int64_t)m_sampleBuffer[i - 1].channels[ch] - zeroValue;
                        const int64_t &x1 = (int64_t)m_sampleBuffer[i].channels[ch] - zeroValue;
                        const uint64_t &t0 = m_sampleBuffer[i - 1].timestampUsec;
                        const uint64_t &t1 = m_sampleBuffer[i].timestampUsec;

                        if (isPositive(x0) != isPositive(x1)) {
                            ++countZeroCrossings;
                            auto t = zeroCrossingTime(t0, x0, t1, x1);
                            firstCrossingTime = std::min(firstCrossingTime, t);
                            lastCrossingTime = std::max(lastCrossingTime, t);
                        }
                    }

                    if (countZeroCrossings == 0) {
                        currEstimation.frequencies[ch] = 0;
                    } else {
                        auto crossingWindowSec =
                                (Float)(lastCrossingTime - firstCrossingTime) / TimeDenom;
                        auto residueSec = (Float)1.0 - crossingWindowSec;

                        /// 2 zero crossings per cycle + 1 crossing starts the count
                        Float freq = (std::max(countZeroCrossings, (uint64_t)1) - 1) / (Float)2.0;

                        /// Adjust the frequency to account for the window being less than 1 second
                        freq *= (1.0 + residueSec);

                        currEstimation.frequencies[ch] = freq;
                    }
                }

                { /// ROCOF estimation
                    auto fdelta = currEstimation.frequencies[ch] - prevEstimation.frequencies[ch];
                    auto tdelta = (currSample.timestampUsec - prevSample.timestampUsec) / TimeDenom;
                    currEstimation.rocofs[ch] = fdelta / tdelta;
                }
            }

            { /// Sampling rate estimation
                auto samplesWindowSec =
                        (Float)(sample.timestampUsec - m_windowStartTime) / TimeDenom;
                auto residueSec = (Float)1.0 - samplesWindowSec;
                size_t countSamples = m_sampleBufIdx + 1;

                /// Adjust the sampling rate to the window, because it is not exactly 1 second
                currEstimation.samplingRate = countSamples * (1.0 + residueSec);
            }

            { /// Reset window variables
                m_sampleBufIdx = m_sampleBuffer.size() - 1;
                m_windowStartTime = sample.timestampUsec;
                m_windowEndTime = m_windowStartTime + TimeDenom;
            }
        }
    }

    // Update the indexes
    m_sampleBufIdx = (m_sampleBufIdx + 1) % m_sampleBuffer.size();
    m_estimationBufIdx = (m_estimationBufIdx + 1) % m_estimationBuffer.size();
}

#undef NEXT
#undef PREV

#undef ESTIMATION_NEXT
#undef ESTIMATION_PREV

#undef SAMPLE_NEXT
#undef SAMPLE_PREV