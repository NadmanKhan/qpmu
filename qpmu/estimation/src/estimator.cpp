#include "qpmu/estimator.h"
#include "qpmu/common.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iomanip>
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

constexpr qpmu::Float zero_crossing_time(qpmu::Float t0, qpmu::Float x0, qpmu::Float t1,
                                         qpmu::Float x1)
{
    return t0 + x0 * (t1 - t0) / (x1 - x0);
}

qpmu::Estimator::~Estimator()
{
    if (m_phasor_strategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < NumChannels; ++i) {
            FFTW<Float>::destroy_plan(m_fftw_state.plans[i]);
            FFTW<Float>::free(m_fftw_state.inputs[i]);
            FFTW<Float>::free(m_fftw_state.outputs[i]);
        }
    }
    // The SDFT state is trivially destructible
}

qpmu::Estimator::Estimator(USize window_size,
                           std::array<std::pair<Float, Float>, NumChannels> calib_params,
                           PhasorEstimationStrategy strategy,
                           FrequencyEstimationStrategy freq_strategy)
    : m_phasor_strategy(strategy),
      m_freq_strategy(freq_strategy),
      m_size(window_size),
      m_calib_params(calib_params),
      m_samples(window_size),
      m_estimations(window_size)
{
    assert(m_size > 2);
    if (m_phasor_strategy == PhasorEstimationStrategy::FFT) {
        for (USize i = 0; i < NumChannels; ++i) {
            m_fftw_state.inputs[i] = FFTW<Float>::alloc_complex(m_size);
            m_fftw_state.outputs[i] = FFTW<Float>::alloc_complex(m_size);
            m_fftw_state.plans[i] =
                    FFTW<Float>::plan_dft_1d(m_size, m_fftw_state.inputs[i],
                                             m_fftw_state.outputs[i], FFTW_FORWARD, FFTW_ESTIMATE);
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

    if (m_freq_strategy == FrequencyEstimationStrategy::TimeBoundZeroCrossings) {
        m_tbzc_xs.resize(2 * 50 * m_size);
        m_tbzc_ts.resize(2 * 50 * m_size);
    }
}

qpmu::Synchrophasor qpmu::Estimator::synchrophasor() const
{
    return m_estimations[m_index];
}

void qpmu::Estimator::update_estimation(qpmu::Sample sample)
{
    using namespace qpmu;

#define NEXT(index) (((index) != (m_size - 1)) * ((index) + 1))
#define PREV(index) (((index) != 0) * ((index)-1) + ((index) == 0) * (m_size - 1))

    const auto &prv = m_estimations[PREV(m_index)];
    auto &cur = m_estimations[m_index];
    cur.timestamp_us = sample.timestampMicrosec;

    for (USize i = 0; i < NumChannels; ++i) {
        const auto &[scale, offset] = m_calib_params[i];
        sample.channel[i] = (sample.channel[i] * scale) + offset;
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
            m_fftw_state.inputs[i][m_size - 1][0] = sample.channel[i];
            m_fftw_state.inputs[i][m_size - 1][1] = 0;
        }
        // Execute the FFT plan
        for (USize i = 0; i < NumChannels; ++i) {
            FFTW<Float>::execute(m_fftw_state.plans[i]);
        }
        // Phasor = output corresponding to the fundamental frequency
        for (USize i = 0; i < NumChannels; ++i) {
            const auto &phasor =
                    Complex(m_fftw_state.outputs[i][1][0], m_fftw_state.outputs[i][1][1])
                    / Float(m_size);
            cur.phasor_mag[i] = std::abs(phasor);
            cur.phasor_ang[i] = std::arg(phasor);
        }

    } else if (m_phasor_strategy == PhasorEstimationStrategy::SDFT) {
        // Run the SDFT on the new sample
        for (USize i = 0; i < NumChannels; ++i) {
            m_sdft_state.workers[i].sdft(sample.channel[i], m_sdft_state.outputs[i].data());
        }
        // Phasor = output corresponding to the fundamental frequency
        for (USize i = 0; i < NumChannels; ++i) {
            const auto &phasor = m_sdft_state.outputs[i][1] / Float(m_size);
            cur.phasor_mag[i] = std::abs(phasor);
            cur.phasor_ang[i] = std::arg(phasor);
        }
    } else {
        std::cerr << "Unknown phasor estimation strategy\n";
        std::abort();
    }

    /**
     * ***************************************************************************************
     * Estimate frequency and ROCOF from the window
     *
     * ***************************************************************************************
     */

    if (m_freq_strategy == FrequencyEstimationStrategy::PhaseDifferences) {
        cur.freq = 0;
        USize cnt = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (Signals[i].type != Signal::Type::Voltage) {
                continue;
            }
            ++cnt;
            auto phase_diff = cur.phasor_ang[i] - prv.phasor_ang[i];
            phase_diff = std::fmod(phase_diff + Float(2 * M_PI), Float(2 * M_PI));
            cur.freq += phase_diff;
        }
        cur.freq /= cnt;
        cur.freq /= sample.timeDeltaMicrosec;
        cur.freq *= 1e6; // sample.delta is in microseconds
        cur.freq /= (2 * M_PI); // Convert from angular frequency to frequency

    } else if (m_freq_strategy == FrequencyEstimationStrategy::SamePhaseCrossings) {
        // Find two phasors with equal phase, and count zero crossings in between
        cur.freq = 0;
        USize cnt_calcs = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (Signals[i].type != Signal::Type::Voltage) {
                continue;
            }
            for (USize j = 0; j < m_estimations.size(); ++j) {
                if (m_estimations[j].timestamp_us == 0) {
                    continue;
                }
                USize cnt_zc = 0;

                auto phase_j = m_estimations[j].phasor_ang[i];
                I64 t0 = m_estimations[j].timestamp_us;

                for (USize k = j + 1; k < m_estimations.size(); ++k) {
                    if (m_estimations[k].timestamp_us == 0) {
                        continue;
                    }

                    const auto &phase_k = m_estimations[k].phasor_ang[i];
                    const auto &phase_prev_k = m_estimations[k - 1].phasor_ang[i];

                    // Check if there is a zero crossing, and increment the count
                    if (is_positive(phase_k) != is_positive(phase_prev_k)) {
                        ++cnt_zc;
                    }

                    // Check if phase_i and phase_j are almost equal
                    auto phase_diff = std::abs(phase_j - phase_k);
                    phase_diff =
                            std::min(phase_diff, std::abs(static_cast<Float>(M_PI) - phase_diff));
                    static constexpr Float MaxAllowedPhaseDiff = (M_PI / 180) * 2; // 2 degrees
                    if (phase_diff <= MaxAllowedPhaseDiff) {
                        I64 t1 = m_estimations[k].timestamp_us;
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
        Float sum_delta_t_sec = 0;
        USize cnt_calcs = 0;
        for (USize i = 0; i < NumChannels; ++i) {
            if (!(Signals[i].type == Signal::Type::Voltage)) {
                continue;
            }
            Float t_last = 0;
            for (USize j = NEXT(m_index); j != m_index; j = NEXT(j)) {
                if (m_estimations[j].timestamp_us == 0) {
                    continue;
                }

                const auto &x0 = m_estimations[PREV(j)].phasor_ang[i];
                const auto &x1 = m_estimations[j].phasor_ang[i];

                if (is_positive(x0) != is_positive(x1)) {
                    // Linear interpolation to find the zero crossing
                    const auto &t0 = m_estimations[PREV(j)].timestamp_us;
                    const auto &t1 = m_estimations[j].timestamp_us;
                    auto t = zero_crossing_time(t0, x0, t1, x1);

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
    } else if (m_freq_strategy == FrequencyEstimationStrategy::TimeBoundZeroCrossings) {
        if (m_tbzc_start_micros == 0) {
            m_tbzc_start_micros = sample.timestampMicrosec;
            m_tbzc_second_mark_micros = m_tbzc_start_micros + (USize)1e6;
        }

        if (m_tbzc_second_mark_micros < sample.timestampMicrosec) {
            // One second has passed; calculate the frequency
            const auto max_value =
                    *std::max_element(m_tbzc_xs.begin(), m_tbzc_xs.begin() + m_tbzc_ptr);
            const Float median_value = max_value / 2.0;

            Float first_zc_micros = 0;
            Float last_zc_micros = 0;
            for (USize i = 1; i < m_tbzc_ptr; ++i) {
                const auto &x0 = m_tbzc_xs[i - 1] - median_value;
                const auto &x1 = m_tbzc_xs[i] - median_value;
                const auto &t0 = m_tbzc_ts[i - 1];
                const auto &t1 = m_tbzc_ts[i];

                if (is_positive(x0) != is_positive(x1)) {
                    ++m_tbzc_count_zc;
                    auto t = zero_crossing_time(t0, x0, t1, x1);
                    if (first_zc_micros == 0) {
                        first_zc_micros = t;
                    }
                    last_zc_micros = t;
                }
            }

            auto time_window_s = (last_zc_micros - first_zc_micros) * 1e-6;
            auto time_residue_s = 1.0 - time_window_s;

            Float f = (m_tbzc_count_zc - 1) / 2.0; // 2 zero crossings per cycle
            cur.freq = f + (time_residue_s * f);

            // Reset the time-bound zero crossing variables
            m_tbzc_ptr = 0;
            m_tbzc_start_micros = sample.timestampMicrosec;
            m_tbzc_second_mark_micros = m_tbzc_start_micros + (USize)1e6;
            m_tbzc_count_zc = 0;
        } else {
            cur.freq = prv.freq;
        }

        m_tbzc_xs[m_tbzc_ptr] = sample.channel[0];
        m_tbzc_ts[m_tbzc_ptr] = sample.timestampMicrosec;
        ++m_tbzc_ptr;

    } else {
        std::cerr << "Unknown frequency estimation strategy\n";
        std::abort();
    }

    cur.rocof = (cur.freq - prv.freq) / sample.timeDeltaMicrosec;

    // Update the index
    m_index = NEXT(m_index);

#undef NEXT
#undef PREV
}