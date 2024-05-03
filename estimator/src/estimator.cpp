#include "qpmu/estimator.h"
#include "qpmu/common.h"
#include <cmath>
#include <iostream>
#include <numeric>
#include <qpmu/util.h>

qpmu::Estimator::~Estimator()
{
    if (m_strategy == EstimationStrategy::FFT) {
        for (SizeType i = 0; i < NumChannels; ++i) {
            FFTW<FloatType>::destroy_plan(m_fftw_state.plans[i]);
            FFTW<FloatType>::free(m_fftw_state.inputs[i]);
            FFTW<FloatType>::free(m_fftw_state.outputs[i]);
        }
    }
    // The SDFT state is trivially destructible
}

qpmu::Estimator::Estimator(SizeType window_size, EstimationStrategy strategy)
    : m_strategy(strategy), m_size(window_size), m_samples(window_size), m_measurements(window_size)
{
    if (m_strategy == EstimationStrategy::FFT) {
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_fftw_state.inputs[i] = FFTW<FloatType>::alloc_complex(m_size);
            m_fftw_state.outputs[i] = FFTW<FloatType>::alloc_complex(m_size);
            m_fftw_state.plans[i] = FFTW<FloatType>::plan_dft_1d(m_size, m_fftw_state.inputs[i],
                                                                 m_fftw_state.outputs[i],
                                                                 FFTW_FORWARD, FFTW_ESTIMATE);
            for (SizeType j = 0; j < m_size; ++j) {
                m_fftw_state.inputs[i][j][0] = 0;
                m_fftw_state.inputs[i][j][1] = 0;
            }
        }
    } else if (m_strategy == EstimationStrategy::SDFT) {
        m_sdft_state.workers =
                std::vector<SdftType>(NumChannels, SdftType(m_size, sdft::Window::Hann, 1));
        m_sdft_state.outputs.assign(NumChannels, std::vector<ComplexType>(m_size, 0));
    }
}

qpmu::Measurement qpmu::Estimator::estimate_measurement(const qpmu::AdcSample &sample)
{
    using namespace qpmu;
#define NEXT(index) (((index) != (m_size - 1)) * ((index) + 1))
#define PREV(index) (((index) != 0) * ((index)-1) + ((index) == 0) * (m_size - 1))

    const auto &prv = m_measurements[PREV(m_index)];
    auto &cur = m_measurements[m_index];

    cur.timestamp = sample.ts;

    /**
     * ***************************************************************************************
     * Add the new sample's data; estimate phasors from the window using the chosen strategy
     * ***************************************************************************************
     */

    if (m_strategy == EstimationStrategy::FFT) {
        // Shift the previous inputs
        for (SizeType i = 0; i < NumChannels; ++i) {
            for (SizeType j = 1; j < m_size; ++j) {
                m_fftw_state.inputs[i][j - 1][0] = m_fftw_state.inputs[i][j][0];
                m_fftw_state.inputs[i][j - 1][1] = m_fftw_state.inputs[i][j][1];
            }
        }
        // Add the new sample's data
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_fftw_state.inputs[i][m_size - 1][0] = sample.ch[i];
            m_fftw_state.inputs[i][m_size - 1][1] = 0;
        }
        // Execute the FFT plan
        for (SizeType i = 0; i < NumChannels; ++i) {
            FFTW<FloatType>::execute(m_fftw_state.plans[i]);
        }
        // Find the phasor with the maximum magnitude (or norm, to do less computation)
        for (SizeType i = 0; i < NumChannels; ++i) {
            const auto &out = m_fftw_state.outputs[i];
            SizeType max_norm_index = 1;
            FloatType max_norm = 0;
            for (SizeType j = 1, half = m_size >> 1; j < half; ++j) {
                FloatType norm = (out[j][0] * out[j][0]) + (out[j][1] * out[j][1]);
                if (norm > max_norm) {
                    max_norm = norm;
                    max_norm_index = j;
                }
            }
            cur.phasors[i] = ComplexType(out[max_norm_index][0], out[max_norm_index][1]);
        }

    } else if (m_strategy == EstimationStrategy::SDFT) {
        // Run the SDFT on the new sample
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_sdft_state.workers[i].sdft(sample.ch[i], m_sdft_state.outputs[i].data());
        }
        // Find the phasor with the maximum magnitude (or norm, to do less computation)
        for (SizeType i = 0; i < NumChannels; ++i) {
            const auto &out = m_sdft_state.outputs[i];
            SizeType max_norm_index = 1;
            FloatType max_norm = 0;
            for (SizeType j = 1, half = m_size >> 1; j < half; ++j) {
                FloatType norm = std::norm(out[j]);
                if (norm > max_norm) {
                    max_norm = norm;
                    max_norm_index = j;
                }
            }
            cur.phasors[i] = out[max_norm_index];
            // scale the phasor down by the window size
            cur.phasors[i] /= FloatType(m_size);
        }
    }

    /**
     * ***************************************************************************************
     * Estimate frequency and ROCOF from the window
     * ---------------------------------------------------------------------------------------
     *
     * Formula:
     * - Frequency = #cycles / delta(time) = (delta(phase) / 2PI) / delta(time)
     * - ROCOF = delta(freq) / delta(time)
     *
     * Take the average of all channels and all elements in the window
     * ***************************************************************************************
     */

    cur.freq = 0;
    for (SizeType i = 0; i < NumChannels; ++i) {
        auto phase_diff = std::arg(cur.phasors[i]) - std::arg(prv.phasors[i]);
        phase_diff = std::fmod(phase_diff, FloatType(2 * M_PI));
        phase_diff = std::fmod(phase_diff + FloatType(2 * M_PI), FloatType(2 * M_PI));
        cur.freq += phase_diff;
    }
    cur.freq /= NumChannels;
    cur.freq /= sample.delta;
    cur.freq *= 1e6; // Because sample.delta is in microseconds
    cur.freq /= (2 * M_PI);
    cur.freq = std::accumulate(m_measurements.begin(), m_measurements.end(), FloatType(0.0),
                               [](FloatType acc, const Measurement &m) { return acc + m.freq; })
            / m_size;

    cur.rocof = (cur.freq - prv.freq) / sample.delta;
    cur.rocof = std::accumulate(m_measurements.begin(), m_measurements.end(), FloatType(0.0),
                                [](FloatType acc, const Measurement &m) { return acc + m.rocof; })
            / m_size;

    // Update the index
    m_index = NEXT(m_index);

    return cur;

#undef NEXT
#undef PREV
}