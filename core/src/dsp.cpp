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

    inline bool push_sample(const Sample &new_sample) noexcept
    {

        const std::size_t prev1 = index_prev(_head);
        const std::size_t prev2 = index_prev(prev1);

        // Validate timestamp
        if (_samples[prev1].timestamp != 0 && new_sample.timestamp <= _samples[prev1].timestamp) {
            std::snprintf(_error, sizeof(_error),
                          "Timestamps must be strictly increasing: previous = %lld, current = %lld",
                          _samples[prev1].timestamp, new_sample.timestamp);
            return false;
        }

        const auto &tail_sample = _samples[_tail];
        const auto &prev_estimation = _estimations[prev1];
        const auto &prev_prev_estimation = _estimations[prev2];

        auto &head_sample = _samples[_head];
        auto &head_estimation = _estimations[_head];

        // Store new sample in circular buffer
        head_sample = new_sample;

        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            Per_Channel_State &s = _channel_states[channel];

            // Phasor estimation via Sliding DFT
            auto phasor = _twiddle_factor // Rotate previous phasor
                    * (prev_estimation.phasors[channel]
                       - static_cast<Float>(tail_sample.values[channel]) // - Outgoing sample
                       + static_cast<Float>(head_sample.values[channel]) // + Incoming sample
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
            auto period = (head_sample.timestamp - tail_sample.timestamp) / One_Second_Period;
            auto frequency = F_Nominal + cycles_diff / period; // in Hz

            // ROCOF estimation via linear regression over the last 3 samples
            auto rocof = linear_regression<Float, 3, Slope_Only>(
                    {
                            Float(0),
                            Float(1),
                            Float(2),
                    },
                    {
                            prev_prev_estimation.frequencies[channel],
                            prev_estimation.frequencies[channel],
                            Float(frequency),
                    });

            // Store estimations
            head_estimation.phasors[channel] = phasor;
            head_estimation.frequencies[channel] = frequency;
            head_estimation.rocofs[channel] = rocof;
        }

        // Advance circular buffer indices
        _head = index_next(_head);
        _tail = index_next(_tail);

        return true;
    }

    inline const Estimation &estimation() const noexcept { return _estimations[index_prev(_head)]; }
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

    // State
    Sample _samples[Buffer_Length] = {};
    Estimation _estimations[Buffer_Length] = {};
    char _error[256] = {};

    // Constants
    Complex _twiddle_factor; // Rotates phasor by 2pi/N per sample

    // Per-channel state; struct-of-arrays for cache efficiency
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
