#include <cassert>

#include "qpmu/algorithms/linear_algebra.hpp"
#include "qpmu/core.h"

namespace qpmu {

template <std::size_t _F0, std::size_t _Fs>
class DSP_Engine
{
public:
    static_assert(_Fs > 2 * _F0, "Sampling frequency must satisfy Nyquist criterion");
    static_assert(_Fs % _F0 == 0,
                  "Sampling frequency must be an integer multiple of nominal frequency");
    static constexpr std::size_t F_Sampling = _Fs; // Sampling frequency, Hz
    static constexpr std::size_t F_Nominal = _F0; // Nominal frequency, Hz (e.g., 50 or 60 Hz)
    static constexpr std::size_t N = _Fs / _F0; // Samples per cycle
    static constexpr std::size_t Buffer_Length = 4 * N; // Length of all circular buffers

    DSP_Engine() noexcept
    {
        // Precompute twiddle factor for Sliding DFT
        _twiddle_factor = std::polar<Float>(1.0, 2.0 * M_PI / Float(N));

        // Initialize circular buffer indices
        _head = 0;
        _tail = Buffer_Length - N;
    }

    inline bool push_reading(const Reading &new_reading) noexcept
    {

        const std::size_t prev1 = index_prev(_head);
        const std::size_t prev2 = index_prev(prev1);

        // Validate timestamp
        if (_readings[prev1].timestamp != 0
            && new_reading.timestamp <= _readings[prev1].timestamp) {
            std::snprintf(_error, sizeof(_error),
                          "Timestamps must be strictly increasing: previous = %lld, current = %lld",
                          _readings[prev1].timestamp, new_reading.timestamp);
            return false;
        }

        const auto &tail_reading = _readings[_tail];
        const auto &prev_estimate = _estimates[prev1];
        const auto &prev_prev_estimate = _estimates[prev2];

        auto &head_reading = _readings[_head];
        auto &head_estimate = _estimates[_head];

        // Store new sample in circular buffer
        head_reading = new_reading;

        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            Per_Channel_State &s = _channel_states[channel];

            // Phasor estimation via Sliding DFT
            auto phasor = _twiddle_factor // Rotate previous phasor
                    * (prev_estimate.phasors[channel]
                       - static_cast<Float>(tail_reading.samples[channel]) // - Outgoing sample
                       + static_cast<Float>(head_reading.samples[channel]) // + Incoming sample
                    );

            // Frequency estimation via discrete differentiation over sliding window
            // - Phase deviation computation
            auto phase = std::arg(phasor);
            s.phase_diffs[_head] = phase - s.prev_phase, s.prev_phase = phase;
            // Wrap phase deviation to [-pi, pi)
            if (s.phase_diffs[_head] >= M_PI)
                s.phase_diffs[_head] -= 2.0 * M_PI;
            else if (s.phase_diffs[_head] < -M_PI)
                s.phase_diffs[_head] += 2.0 * M_PI;
            s.window_sum_phase_diff -= s.phase_diffs[_tail]; // - Outgoing phase deviation
            s.window_sum_phase_diff += s.phase_diffs[_head]; // + Incoming phase deviation
            // - Frequency computation
            auto cycles_diff = s.window_sum_phase_diff / (2.0 * M_PI);
            auto period = (head_reading.timestamp - tail_reading.timestamp) / Time_Resolutiion;
            auto frequency = F_Nominal + cycles_diff / period; // in Hz

            // ROCOF estimation via linear regression over the last 3 samples
            auto rocof = linear_regression<Float, 3, Slope_Only>(
                    {
                            Float(0),
                            Float(1),
                            Float(2),
                    },
                    {
                            prev_prev_estimate.frequencies[channel],
                            prev_estimate.frequencies[channel],
                            Float(frequency),
                    });

            // Store estimations
            head_estimate.phasors[channel] = phasor;
            head_estimate.frequencies[channel] = frequency;
            head_estimate.rocofs[channel] = rocof;
        }

        // Advance circular buffer indices
        _head = index_next(_head);
        _tail = index_next(_tail);

        return true;
    }

    inline const Estimate &estimate() const noexcept { return _estimates[index_prev(_head)]; }
    inline const char *error() const noexcept { return _error; }

private:
    static constexpr std::size_t index_next(std::size_t i) noexcept
    {
        return (i + 1 == Buffer_Length) ? 0 : (i + 1);
    }
    static constexpr std::size_t index_prev(std::size_t i) noexcept
    {
        return (i == 0) ? (Buffer_Length - 1) : (i - 1);
    }

    // Constants
    Complex _twiddle_factor; // Rotates phasor by 2pi/N per sample

    // State
    Reading _readings[Buffer_Length] = {};
    Estimate _estimates[Buffer_Length] = {};
    char _error[256] = {};
    struct Per_Channel_State
    {
        Float prev_phase = 0.0;
        Float phase_diffs[Buffer_Length] = {};
        Float window_sum_phase_diff = 0.0;
    };
    Per_Channel_State _channel_states[N_Channels] = {};
    std::size_t _head = 0; // One past the last valid position; next to write
    std::size_t _tail = 0; // Oldest valid position (same as (head - N) mod Len_Buffer)
};

} // namespace qpmu
