#include <cassert>
#include <cmath>
#include <complex>
#include <cstddef>

#include "qpmu/containers/sliding_median.hpp"
#include "qpmu/core.h"
#include "qpmu/utilities/modulo.hpp"

namespace qpmu {

template <std::size_t _F0, std::size_t _Fs>
class DSP_Engine
{
public:
    static_assert(_Fs > 2 * _F0, "Sampling frequency must satisfy Nyquist criterion");
    static_assert(_Fs % _F0 == 0,
                  "Sampling frequency must be an integer multiple of nominal frequency");
    static constexpr std::size_t F_Sampling = _Fs;
    static constexpr std::size_t F_Nominal = _F0;
    static constexpr std::size_t N = _Fs / _F0;
    static constexpr std::size_t Circular_Buffer_Length = 4 * N;
    static constexpr std::size_t Num_SDFT_Bins = 4; // k = 0, 1, 2, 3

    explicit DSP_Engine(const DSP_Config &config = {}) noexcept : _config(config)
    {
        for (std::size_t k = 0; k < Num_SDFT_Bins; ++k) {
            _twiddle[k] = std::polar<Float>(1.0f, 2.0f * Float(M_PI) * Float(k) / Float(N));
        }

        _head = 0;
        _tail = Circular_Buffer_Length - N;

        for (auto &ch : _channels) {
            ch.freq_median = Sliding_Median<Float>(config.freq_median_window);
            ch.rocof_median = Sliding_Median<Float>(config.rocof_median_window);
        }
    }

    inline bool push_sample_frame(const Sample_Frame &sample_frame) noexcept
    {
        const std::size_t prev1 = modulo_prev<std::size_t, Circular_Buffer_Length>(_head);

        if (_sample_frames_circbuf[prev1].timestamp != 0
            && sample_frame.timestamp <= _sample_frames_circbuf[prev1].timestamp) {
            std::snprintf(_error, sizeof(_error),
                          "Timestamps must be strictly increasing: previous = %lld, current = %lld",
                          _sample_frames_circbuf[prev1].timestamp, sample_frame.timestamp);
            return false;
        }

        _sample_frames_circbuf[_head] = sample_frame;

        const auto &head_sf = _sample_frames_circbuf[_head];
        const auto &tail_sf = _sample_frames_circbuf[_tail];

        for (std::size_t ci = 0; ci < Signal_Infos.size(); ++ci) {
            Channel_State &channel = _channels[ci];
            Estimate &estimate = channel.estimates_circbuf[_head];

            // --- DC offset removal ---
            Float centered_head, centered_tail;
            {
                Float raw_head = Float(head_sf.sample_array[ci]);
                if (_config.dc_alpha > 0) {
                    channel.dc_estimate += _config.dc_alpha * (raw_head - channel.dc_estimate);
                    centered_head = raw_head - channel.dc_estimate;
                    centered_tail = channel.centered_circbuf[_tail];
                } else {
                    centered_head = raw_head;
                    centered_tail = Float(tail_sf.sample_array[ci]);
                }
                channel.centered_circbuf[_head] = centered_head;
            }

            // --- Multi-bin SDFT update (k = 0, 1, 2, 3) ---
            auto &bins = channel.sdft_bins_circbuf[_head];
            const auto &prev_bins = channel.sdft_bins_circbuf[prev1];
            Float sample_delta = centered_head - centered_tail;
            for (std::size_t k = 0; k < Num_SDFT_Bins; ++k) {
                bins[k] = _twiddle[k] * (prev_bins[k] + sample_delta);
            }

            // --- Phasor extraction + frequency estimation ---
            if (_config.ipdft_enabled && _config.hann_enabled) {
                // Hann-windowed bins at k=1 and k=2
                // X_hann[k] = -0.25*X_rect[k-1] + 0.5*X_rect[k] - 0.25*X_rect[k+1]
                // Note: X_hann[0] is unusable due to negative-frequency image
                // contamination (bin N-1 = conj(bin 1) leaks into bin 0).
                // Always interpolate between bins 1 and 2 only.
                Complex x_hann_1 = Float(-0.25f) * bins[0]
                                 + Float(0.5f)   * bins[1]
                                 + Float(-0.25f) * bins[2];
                Complex x_hann_2 = Float(-0.25f) * bins[1]
                                 + Float(0.5f)   * bins[2]
                                 + Float(-0.25f) * bins[3];

                Float mag1 = std::abs(x_hann_1);
                Float mag2 = std::abs(x_hann_2);

                // 2-point Hann IpDFT using bins 1 and 2
                // alpha = 0.5 when signal is exactly at F0 (delta = 0)
                // alpha < 0.5 => delta < 0 (freq below F0)
                // alpha > 0.5 => delta > 0 (freq above F0)
                Float alpha = mag2 / (mag1 + 1e-30f);
                Float delta = (2.0f * alpha - 1.0f) / (alpha + 1.0f);

                // Corrected frequency
                estimate.frequency = (1.0f + delta) * Float(F_Nominal);

                // Corrected amplitude (scalloping loss + DFT normalization)
                Float abs_delta = std::abs(delta);
                Float amplitude;
                if (abs_delta < 1e-6f) {
                    amplitude = mag1 * (2.0f / Float(N));
                } else {
                    amplitude = mag1 * Float(M_PI) * delta * (1.0f - delta * delta)
                              / std::sin(Float(M_PI) * delta) * (2.0f / Float(N));
                }
                if (amplitude < 0)
                    amplitude = -amplitude;

                // Corrected phase
                Float phase = std::arg(x_hann_1)
                            - Float(M_PI) * delta * Float(N - 1) / Float(N);
                estimate.phasor = std::polar(amplitude, phase);

            } else {
                // Hann or rectangular phasor, phase-displacement frequency
                Complex phasor;
                if (_config.hann_enabled) {
                    phasor = Float(-0.25f) * bins[0]
                           + Float(0.5f) * bins[1]
                           + Float(-0.25f) * bins[2];
                    estimate.phasor = phasor * (2.0f / Float(N));
                } else {
                    phasor = bins[1];
                    estimate.phasor = phasor / Float(N);
                }

                // Frequency via phase-displacement accumulation
                const Float curr_phase = std::arg(estimate.phasor);
                channel.phase_diffs_circbuf[_head] =
                        wrapping_add(curr_phase, -channel.prev_phase,
                                     -Float(M_PI), Float(M_PI));
                channel.prev_phase = curr_phase;

                channel.running_total_phase_displacement += channel.phase_diffs_circbuf[_head];
                channel.running_total_phase_displacement -= channel.phase_diffs_circbuf[_tail];
                const Float cycles =
                        channel.running_total_phase_displacement / (2.0f * Float(M_PI));
                const Float period =
                        Float(head_sf.timestamp - tail_sf.timestamp) / Float(Time_Resolution);
                estimate.frequency = cycles / period;
            }

            // --- Frequency median filter ---
            if (channel.freq_median.enabled()) {
                channel.freq_median.push(estimate.frequency);
                estimate.frequency = channel.freq_median.median();
            }

            // --- ROCOF via runtime-sized OLS on uniform x-spacing ---
            {
                auto &hist = channel.freq_history;
                std::size_t M_max = _config.rocof_regression_window;
                hist[channel.freq_history_pos] = estimate.frequency;
                channel.freq_history_pos = (channel.freq_history_pos + 1) % M_max;
                if (channel.freq_history_count < M_max)
                    ++channel.freq_history_count;

                if (channel.freq_history_count >= 3) {
                    const std::size_t M = channel.freq_history_count;
                    Float sum_y = 0;
                    Float sum_iy = 0;
                    for (std::size_t j = 0; j < M; ++j) {
                        std::size_t idx = (channel.freq_history_pos + M_max - M + j) % M_max;
                        Float y = hist[idx];
                        sum_y += y;
                        sum_iy += Float(j) * y;
                    }
                    // OLS slope for uniformly-spaced x = {0, 1, ..., M-1}
                    Float slope = (12.0f * sum_iy - 6.0f * Float(M - 1) * sum_y)
                                / (Float(M) * (Float(M) * Float(M) - 1.0f));
                    estimate.rocof = slope * Float(F_Sampling);
                } else {
                    estimate.rocof = 0;
                }
            }

            // --- ROCOF median filter ---
            if (channel.rocof_median.enabled()) {
                channel.rocof_median.push(estimate.rocof);
                estimate.rocof = channel.rocof_median.median();
            }
        }

        // Copy estimates to output
        _measurement_frame.seq_num = sample_frame.seq_num;
        _measurement_frame.timestamp = sample_frame.timestamp;
        _measurement_frame.sample_array = sample_frame.sample_array;
        for (std::size_t ci = 0; ci < Signal_Infos.size(); ++ci) {
            _measurement_frame.estimate_array[ci] = _channels[ci].estimates_circbuf[_head];
        }

        _head = modulo_next<std::size_t, Circular_Buffer_Length>(_head);
        _tail = modulo_next<std::size_t, Circular_Buffer_Length>(_tail);

        return true;
    }

    inline const Measurement_Frame &measurement_frame() const noexcept
    {
        return _measurement_frame;
    }
    inline const char *error() const noexcept { return _error; }

private:
    const DSP_Config _config;
    std::array<Complex, Num_SDFT_Bins> _twiddle;

    Measurement_Frame _measurement_frame = {};
    char _error[256] = {};

    std::size_t _head = 0;
    std::size_t _tail = 0;
    std::array<Sample_Frame, Circular_Buffer_Length> _sample_frames_circbuf = {};

    struct Channel_State
    {
        std::array<std::array<Complex, Num_SDFT_Bins>, Circular_Buffer_Length> sdft_bins_circbuf = {};
        std::array<Estimate, Circular_Buffer_Length> estimates_circbuf = {};

        Float dc_estimate = 2048.0f;
        std::array<Float, Circular_Buffer_Length> centered_circbuf = {};

        std::array<Float, Circular_Buffer_Length> phase_diffs_circbuf = {};
        Float prev_phase = 0.0f;
        Float running_total_phase_displacement = 0.0f;

        std::array<Float, 31> freq_history = {};
        std::size_t freq_history_pos = 0;
        std::size_t freq_history_count = 0;

        Sliding_Median<Float> freq_median{ 0 };
        Sliding_Median<Float> rocof_median{ 0 };
    };
    std::array<Channel_State, Signal_Infos.size()> _channels = {};
};

} // namespace qpmu
