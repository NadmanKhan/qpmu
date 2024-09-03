#include "qpmu/estimator.h"
#include "qpmu/common.h"

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

constexpr Float zeroCrossingTime(Float t0, Float x0, Float t1, Float x1)
{
    return t0 + (0 - x0) * (t1 - t0) / (x1 - x0);
}

Estimator::~Estimator()
{
    if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < CountSignals; ++i) {
            FFTW<Float>::destroy_plan(m_fftwState.plans[i]);
            FFTW<Float>::free(m_fftwState.inputs[i]);
            FFTW<Float>::free(m_fftwState.outputs[i]);
        }
    }

    // The SDFT state is trivially destructible
}

Estimator::Estimator(USize fn, USize fs, PhasorEstimationStrategy phasorStrategy)
    : m_phasorStrategy(phasorStrategy)
{
    assert(fn > 0);
    assert(fs % fn == 0);

    m_syncphBuffer.resize(fs / fn); // hold one full cycle (fs / fn = number of samples per cycle)
    m_sampleBuffer.resize(5 * fs); // hold at least 1 second of samples

    if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < CountSignals; ++i) {
            m_fftwState.inputs[i] = FFTW<Float>::alloc_complex(m_syncphBuffer.size());
            m_fftwState.outputs[i] = FFTW<Float>::alloc_complex(m_syncphBuffer.size());
            m_fftwState.plans[i] =
                    FFTW<Float>::plan_dft_1d(m_syncphBuffer.size(), m_fftwState.inputs[i],
                                             m_fftwState.outputs[i], FFTW_FORWARD, FFTW_ESTIMATE);
            for (USize j = 0; j < m_syncphBuffer.size(); ++j) {
                m_fftwState.inputs[i][j][0] = 0;
                m_fftwState.inputs[i][j][1] = 0;
            }
        }

    } else if (m_phasorStrategy == PhasorEstimationStrategy::SDFT) {
        m_sdftState.workers = std::vector<SdftType>(
                CountSignals, SdftType(m_syncphBuffer.size(), sdft::Window::Hann, 1));
        m_sdftState.outputs.assign(CountSignals, std::vector<Complex>(m_syncphBuffer.size(), 0));
    }
}

#define NEXT(i, x) (((i) != (m_##x##Buffer.size() - 1)) * ((i) + 1))
#define PREV(i, x) (((i) != 0) * ((i)-1) + ((i) == 0) * (m_##x##Buffer.size() - 1))

#define SYNCPH_NEXT(i) NEXT(i, syncph)
#define SYNCPH_PREV(i) PREV(i, syncph)

#define SAMPLE_NEXT(i) NEXT(i, sample)
#define SAMPLE_PREV(i) PREV(i, sample)

const Synchrophasor &Estimator::lastSynchrophasor() const
{
    return m_syncphBuffer[m_syncphBufIdx];
}

const Sample &Estimator::lastSample() const
{
    return m_sampleBuffer[m_sampleBufIdx];
}

const std::array<Float, CountSignals> &Estimator::channelMagnitudes() const
{
    return m_channelMagnitudes;
}

void Estimator::updateEstimation(Sample sample)
{
    m_sampleBuffer[m_sampleBufIdx] = sample;

    const Synchrophasor &prevSyncph = m_syncphBuffer[SYNCPH_PREV(m_syncphBufIdx)];
    Synchrophasor &currSyncph = m_syncphBuffer[m_syncphBufIdx];

    currSyncph.timestampUs = sample.timestampUs;

    { /// Estimate phasors

        if (m_phasorStrategy == PhasorEstimationStrategy::FFT) {
            // Shift the previous inputs
            for (USize i = 0; i < CountSignals; ++i) {
                for (USize j = 1; j < m_syncphBuffer.size(); ++j) {
                    m_fftwState.inputs[i][j - 1][0] = m_fftwState.inputs[i][j][0];
                    m_fftwState.inputs[i][j - 1][1] = m_fftwState.inputs[i][j][1];
                }
            }
            // Add the new sample's data
            for (USize i = 0; i < CountSignals; ++i) {
                m_fftwState.inputs[i][m_syncphBuffer.size() - 1][0] = sample.channels[i];
                m_fftwState.inputs[i][m_syncphBuffer.size() - 1][1] = 0;
            }
            // Execute the FFT plan
            for (USize i = 0; i < CountSignals; ++i) {
                FFTW<Float>::execute(m_fftwState.plans[i]);
            }
            // Phasor = output corresponding to the fundamental frequency
            for (USize i = 0; i < CountSignals; ++i) {
                Complex phasor = { m_fftwState.outputs[i][1][0], m_fftwState.outputs[i][1][1] };
                phasor /= Float(m_syncphBuffer.size());
                currSyncph.magnitudes[i] = std::abs(phasor);
                currSyncph.phaseAngles[i] = std::arg(phasor);
            }

        } else if (m_phasorStrategy == PhasorEstimationStrategy::SDFT) {
            // Run the SDFT on the new sample
            for (USize i = 0; i < CountSignals; ++i) {
                m_sdftState.workers[i].sdft(sample.channels[i], m_sdftState.outputs[i].data());
            }
            // Phasor = output corresponding to the fundamental frequency
            for (USize i = 0; i < CountSignals; ++i) {
                const auto &phasor = m_sdftState.outputs[i][1] / Float(m_syncphBuffer.size());
                currSyncph.magnitudes[i] = std::abs(phasor);
                currSyncph.phaseAngles[i] = std::arg(phasor);
            }
        }
    }

    { /// Estimate frequency and ROCOF; update

        if (m_windowStartTimeUs == 0) {
            m_windowStartTimeUs = sample.timestampUs;
            m_windowEndTimeUs = m_windowStartTimeUs + (USize)1e6;
        }

        currSyncph.frequency = prevSyncph.frequency;

        if (m_windowEndTimeUs < sample.timestampUs) {
            /// The 1s window has ended, hence
            /// - estimate frequency a new,
            /// - update channel magnitudes, and
            /// - reset the window variables

            /// Count zero crossings for frequency estimation
            /// ---

            constexpr USize FreqEstimChannel = 0;

            U64 maxValue = 0;
            for (USize i = 0; i <= m_sampleBufIdx; ++i) {
                if (m_sampleBuffer[i].channels[FreqEstimChannel] > maxValue) {
                    maxValue = m_sampleBuffer[i].channels[FreqEstimChannel];
                }
            }

            const U64 zeroValue = maxValue / 2;
            U64 firstCrossingUs = 0;
            U64 lastCrossingUs = 0;

            for (USize i = 1; i <= m_sampleBufIdx; ++i) {
                const I64 &x0 = (I64)m_sampleBuffer[i - 1].channels[FreqEstimChannel] - zeroValue;
                const I64 &x1 = (I64)m_sampleBuffer[i].channels[FreqEstimChannel] - zeroValue;
                const U64 &t0 = m_sampleBuffer[i - 1].timestampUs;
                const U64 &t1 = m_sampleBuffer[i].timestampUs;

                if (isPositive(x0) != isPositive(x1)) {
                    ++m_zeroCrossingCount;
                    auto t = (U64)std::round(zeroCrossingTime(t0, x0, t1, x1));
                    if (firstCrossingUs == 0) {
                        firstCrossingUs = t;
                    }
                    lastCrossingUs = t;
                }
            }

            auto crossingWindowSec = (Float)(lastCrossingUs - firstCrossingUs) * 1e-6;
            auto residueSec = (Float)1.0 - crossingWindowSec;

            /// 2 zero crossings per cycle + 1 crossing starts the count
            Float f = (Float)(m_zeroCrossingCount - 1) / 2.0;

            currSyncph.frequency = f + (residueSec * f);

            /// Update channel magnitudes
            /// ---

            for (USize i = 0; i < CountSignals; ++i) {
                USize sum = 0;
                USize count = 0;

                Sample *s0 = &m_sampleBuffer[0];
                Sample *s1 = &m_sampleBuffer[1];
                Sample *s2 = &m_sampleBuffer[2];
                Sample *s3 = &m_sampleBuffer[3];
                Sample *s4 = &m_sampleBuffer[4];

                for (USize j = 4; j <= m_sampleBufIdx; ++j) {
                    if (s0->channels[i] <= s1->channels[i] && s1->channels[i] <= s2->channels[i]
                        && s2->channels[i] >= s3->channels[i]
                        && s3->channels[i] >= s4->channels[i]) {

                        sum += s2->channels[i];
                        count += 1;
                    }

                    ++s0;
                    ++s1;
                    ++s2;
                    ++s3;
                    ++s4;
                }

                m_channelMagnitudes[i] = (Float)sum / (Float)count;
            }

            /// Reset window variables
            /// ---

            m_sampleBufIdx = 0;
            m_windowStartTimeUs = sample.timestampUs;
            m_windowEndTimeUs = m_windowStartTimeUs + (USize)1e6;
            m_zeroCrossingCount = 0;
        }

        currSyncph.rocof = (currSyncph.frequency - prevSyncph.frequency) / sample.timeDeltaUs * 1e6;
    }

    // Update the indexes
    m_sampleBufIdx += 1;
    m_syncphBufIdx = (m_syncphBufIdx + 1) % m_syncphBuffer.size();
}

#undef NEXT
#undef PREV

#undef SYNCPH_NEXT
#undef SYNCPH_PREV

#undef SAMPLE_NEXT
#undef SAMPLE_PREV