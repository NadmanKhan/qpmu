#include "qpmu/estimator.h"
#include "qpmu/common.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <limits>
#include <numeric>

template <class Numeric>
constexpr bool is_positive(Numeric x)
{
    if constexpr (std::is_integral<Numeric>::value) {
        return x > 0;
    } else {
        return x > 0 + std::numeric_limits<Numeric>::epsilon();
    }
}

qpmu::Estimator::~Estimator()
{
    if (m_phasor_strategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < NumChannels; ++i) {
            FFTW<FloatType>::destroy_plan(m_fftw_state.plans[i]);
            FFTW<FloatType>::free(m_fftw_state.inputs[i]);
            FFTW<FloatType>::free(m_fftw_state.outputs[i]);
        }
    }
    // The SDFT state is trivially destructible
}

qpmu::Estimator::Estimator(USize window_size, PhasorEstimationStrategy strategy,
                           FrequencyEstimationStrategy freq_strategy,
                           std::pair<FloatType, FloatType> voltage_params,
                           std::pair<FloatType, FloatType> current_params)
    : m_phasor_strategy(strategy),
      m_freq_strategy(freq_strategy),
      m_size(window_size),
      m_scale_voltage(voltage_params.first),
      m_offset_voltage(voltage_params.second),
      m_scale_current(current_params.first),
      m_offset_current(current_params.second),
      m_samples(window_size),
      m_estimations(window_size)
{
    assert(m_size > 2);
    if (m_phasor_strategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < NumChannels; ++i) {
            m_fftw_state.inputs[i] = FFTW<FloatType>::alloc_complex(m_size);
            m_fftw_state.outputs[i] = FFTW<FloatType>::alloc_complex(m_size);
            m_fftw_state.plans[i] = FFTW<FloatType>::plan_dft_1d(m_size, m_fftw_state.inputs[i],
                                                                 m_fftw_state.outputs[i],
                                                                 FFTW_FORWARD, FFTW_ESTIMATE);
            for (USize j = 0; j < m_size; ++j) {
                m_fftw_state.inputs[i][j][0] = 0;
                m_fftw_state.inputs[i][j][1] = 0;
            }
        }
    } else if (m_phasor_strategy == PhasorEstimationStrategy::SDFT) {
        m_sdft_state.workers =
                std::vector<SdftType>(NumChannels, SdftType(m_size, sdft::Window::Hann, 1));
        m_sdft_state.outputs.assign(NumChannels, std::vector<Complex>(m_size, 0));
    }
}

qpmu::Estimation qpmu::Estimator::add_estimation(qpmu::AdcSample sample)
{
    using namespace qpmu;
#define NEXT(index) (((index) != (m_size - 1)) * ((index) + 1))
#define PREV(index) (((index) != 0) * ((index)-1) + ((index) == 0) * (m_size - 1))

    const auto &prv = m_estimations[PREV(m_index)];
    // const auto &nxt = m_estimations[NEXT(m_index)];
    // const auto old = m_estimations[m_index];
    auto &cur = m_estimations[m_index];

    for (USize i = 0; i < NumChannels; ++i) {
        const auto &scale = (signal_is_voltage(Signals[i]) ? m_scale_voltage : m_scale_current);
        const auto &offset = (signal_is_voltage(Signals[i]) ? m_offset_voltage : m_offset_current);
        sample.ch[i] = (sample.ch[i] * scale) + offset;
    }

    /**
     * ***************************************************************************************
     * Add the new sample's data; estimate phasors from the window using the chosen strategy
     * ***************************************************************************************
     */

    if (m_phasor_strategy == PhasorEstimationStrategy::FFT) {
        // Shift the previous inputs
        for (USize i = 0; i < NumChannels; ++i) {
            for (USize j = 1; j < m_size; ++j) {
                m_fftw_state.inputs[i][j - 1][0] = m_fftw_state.inputs[i][j][0];
                m_fftw_state.inputs[i][j - 1][1] = m_fftw_state.inputs[i][j][1];
            }
        }
        // Add the new sample's data
        for (USize i = 0; i < NumChannels; ++i) {
            m_fftw_state.inputs[i][m_size - 1][0] = sample.ch[i];
            m_fftw_state.inputs[i][m_size - 1][1] = 0;
        }
        // Execute the FFT plan
        for (USize i = 0; i < NumChannels; ++i) {
            FFTW<FloatType>::execute(m_fftw_state.plans[i]);
        }
        // Phasor = output corresponding to the fundamental frequency
        for (USize i = 0; i < NumChannels; ++i) {
            const auto &phasor = m_fftw_state.outputs[i][1];
            cur.phasors[i] = Complex(phasor[0], phasor[1]);
        }

    } else if (m_phasor_strategy == PhasorEstimationStrategy::SDFT) {
        // Run the SDFT on the new sample
        for (USize i = 0; i < NumChannels; ++i) {
            m_sdft_state.workers[i].sdft(sample.ch[i], m_sdft_state.outputs[i].data());
        }
        // Phasor = output corresponding to the fundamental frequency
        for (USize i = 0; i < NumChannels; ++i) {
            cur.phasors[i] = m_sdft_state.outputs[i][1];
        }
    } else {
        std::cerr << "Unknown phasor estimation strategy\n";
        std::abort();
    }

    // scale the phasors down by the window size
    for (USize i = 0; i < NumChannels; ++i) {
        cur.phasors[i] /= FloatType(m_size);
    }

    /**
     * ***************************************************************************************
     * Estimate frequency and ROCOF from the window
     * ---------------------------------------------------------------------------------------
     *
     * - ROCOF = delta(freq) / delta(time)
     *
     * Formula for PhasorAngle strategy:
     * - Frequency = #cycles / delta(time) = (delta(phase) / 2PI) / delta(time)
     *
     * ***************************************************************************************
     */

    if (m_freq_strategy == FrequencyEstimationStrategy::ConsecutivePhaseDifferences) {
        cur.freq = 0;
        USize cnt = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (!signal_is_voltage(Signals[i])) {
                continue;
            }
            ++cnt;
            auto phase_diff = std::arg(cur.phasors[i]) - std::arg(prv.phasors[i]);
            phase_diff = std::fmod(phase_diff + FloatType(2 * M_PI), FloatType(2 * M_PI));
            cur.freq += phase_diff;
        }
        cur.freq /= cnt;
        cur.freq /= sample.delta;
        cur.freq *= 1e6; // sample.delta is in microseconds
        cur.freq /= (2 * M_PI); // Convert from angular frequency to frequency

    } else if (m_freq_strategy == FrequencyEstimationStrategy::SamePhaseCrossings) {
        // Find two phasors with equal phase, and count zero crossings in between
        cur.freq = 0;
        USize cnt_calcs = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (!signal_is_voltage(Signals[i])) {
                continue;
            }
            for (USize j = 0; j < m_estimations.size(); ++j) {
                if (m_estimations[j].src_sample.ts == 0) {
                    continue;
                }
                USize cnt_zc = 0;

                auto phase_j = std::arg(m_estimations[j].phasors[i]);
                I64 t0 = m_estimations[j].src_sample.ts;

                for (USize k = j + 1; k < m_estimations.size(); ++k) {
                    if (m_estimations[k].src_sample.ts == 0) {
                        continue;
                    }

                    auto phase_k = std::arg(m_estimations[k].phasors[i]);
                    auto phase_prev_k = std::arg(m_estimations[k - 1].phasors[i]);

                    // Check if there is a zero crossing, and increment the count
                    if (is_positive(phase_k) != is_positive(phase_prev_k)) {
                        ++cnt_zc;
                    }

                    // Check if phase_i and phase_j are almost equal
                    auto phase_diff = std::abs(phase_j - phase_k);
                    phase_diff = std::abs(std::min(phase_diff, M_PI - phase_diff));
                    static constexpr FloatType MaxAllowedPhaseDiff = (M_PI / 180) * 2; // 2 degrees
                    if (phase_diff <= MaxAllowedPhaseDiff) {
                        I64 t1 = m_estimations[k].src_sample.ts;
                        auto delta_t_sec = std::abs(t1 - t0) * 1e-6; // sample.ts is in microseconds
                        auto cnt_cycles = cnt_zc / 2.0;

                        cur.freq += cnt_cycles / delta_t_sec;
                        ++cnt_calcs;
                        break;
                    }
                }
            }
        }
        cur.freq /= cnt_calcs;

    } else if (m_freq_strategy == FrequencyEstimationStrategy::ZeroCrossings) {
        // Find (approximate) zero crossings of the signal samples
        FloatType sum_delta_t_sec = 0;
        USize cnt_calcs = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (!signal_is_voltage(Signals[i])) {
                continue;
            }
            I64 max_value = 0;
            for (USize j = 0; j < m_estimations.size(); ++j) {
                max_value = std::max(max_value, (I64)(m_estimations[j].src_sample.ch[i]));
            }
            I64 median_value = max_value / 2;
            FloatType t_last = 0;
            for (USize j = NEXT(m_index); j != m_index; j = NEXT(j)) {
                if (m_estimations[j].src_sample.ts == 0) {
                    continue;
                }
                const auto &v0 = m_estimations[PREV(j)].src_sample.ch[i] - median_value;
                const auto &v1 = m_estimations[j].src_sample.ch[i] - median_value;
                if (is_positive(v0) != is_positive(v1)) {
                    // Linear interpolation to find the zero crossing
                    const auto &t0 = m_estimations[PREV(j)].src_sample.ts;
                    const auto &t1 = m_estimations[j].src_sample.ts;
                    auto t = t0 + v0 * FloatType(t1 - t0) / (v1 - v0);
                    if (t_last != 0) {
                        auto delta_t_sec =
                                std::abs(t - t_last) * 1e-6; // sample.ts is in microseconds
                        sum_delta_t_sec += delta_t_sec;
                        ++cnt_calcs;
                    }
                    t_last = t;
                }
            }
        }
        if (cnt_calcs > 0) {
            cur.freq = cnt_calcs / (2 * sum_delta_t_sec);
        } else {
            cur.freq = 0;
        }

    } else {
        std::cerr << "Unknown frequency estimation strategy\n";
        std::abort();
    }

    cur.rocof = (cur.freq - prv.freq) / sample.delta;

    for (USize p = 0; p < NumPhases; ++p) {
        const auto &[v_index, i_index] = SignalPhasePairs[p];
        cur.power[p] = (cur.phasors[v_index] * std::conj(cur.phasors[i_index])).real();
    }

    // Update the index
    m_index = NEXT(m_index);

    cur.src_sample = sample;

    return cur;

#undef NEXT
#undef PREV
}