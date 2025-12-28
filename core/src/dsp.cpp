#include <cassert>

#include "qpmu/algorithms/linear_algebra.hpp"
#include "qpmu/core.h"
#include "qpmu/containers/sliding_mmm.hpp"
#include "qpmu/utilities/modulo.hpp"

namespace qpmu {

template <std::size_t _F0, std::size_t _Fs>
class DSP_Engine
{
public:
    static_assert(_Fs > 2 * _F0, "Sampling frequency must satisfy Nyquist criterion");
    static_assert(_Fs % _F0 == 0,
                  "Sampling frequency must be an integer multiple of nominal frequency");
    static constexpr std::size_t F_Sampling = _Fs; // Sampling frequency (Hz)
    static constexpr std::size_t F_Nominal = _F0; // Nominal frequency (Hz)
    static constexpr std::size_t N = _Fs / _F0; // Samples per cycle
    static constexpr std::size_t Len_Circular_Buffer = 4 * N; // Length of general circular buffers
    static constexpr std::size_t Len_ZC_Circular_Buffer =
            F_Nominal * 3; // A cycle has two zero crossings, but we use extra space for safety

    DSP_Engine()
    {
        // Precompute twiddle factor for Sliding DFT
        _twiddle_factor = std::polar<Float>(1.0, 2.0 * M_PI / Float(N));

        // Initialize circular buffer indices
        _head = 0;
        _tail = Len_Circular_Buffer - N;
    }

    inline bool push_sample(const Sample &new_sample) noexcept
    {
        using namespace detail;

        std::size_t prev1 = index_prev(_head);
        std::size_t prev2 = index_prev(prev1);

        // Validate timestamp
        const auto &old_sample = _sample;
        if (old_sample.timestamp != 0 && new_sample.timestamp <= old_sample.timestamp) {
            std::ostringstream err;
            std::snprintf(_error, sizeof(_error),
                          "Timestamps must be strictly increasing: previous = %lld, current = %lld",
                          old_sample.timestamp, new_sample.timestamp);
            return false;
        }

        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            Per_Channel_State &state = _channel_state[channel];

            // Phasor estimation via Sliding DFT
            // ---

            state.phasors[_head] = _twiddle_factor // Rotate previous phasor
                    * (state.phasors[prev1]
                       - static_cast<Float>(old_sample.values[channel]) // - Outgoing sample
                       + static_cast<Float>(new_sample.values[channel]) // + Incoming sample
                    );

            // Frequency and ROCOF estimation via zero-crossings
            // ---

            // Check for zero crossing between previous and current sample
            state.samples_heap.push_next(new_sample.values[channel]);
            const auto &minval = state.samples_heap.min();
            const auto &maxval = state.samples_heap.max();
            const std::int64_t &t1 = old_sample.timestamp;
            const std::int64_t &v1 = old_sample.values[channel];
            const std::int64_t &t2 = new_sample.timestamp;
            const std::int64_t &v2 = new_sample.values[channel];
            const std::int64_t v_zero = minval + (maxval - minval) / 2;

            if ((v1 < v_zero && v2 >= v_zero) || (v2 < v_zero && v1 >= v_zero)) {
                // Zero crossing occurred; find its exact timestamp by linear interpolation
                Timestamp t_zero = linear_interpolation(v1, t1, v2, t2, v_zero);
                state.zc_timestamps[state.zc_head] = t_zero;

                // Advance tail until all timestamps are within the last second, and
                // advance head because we have added a new timestamp
                while (range_length_zc(state.zc_tail, state.zc_head) > 1
                       && (t_zero - state.zc_timestamps[state.zc_tail] > One_Second_Period)) {
                    state.zc_tail = index_next_zc(state.zc_tail);
                }
                state.zc_head = index_next_zc(state.zc_head);
            }

            // Estimate frequency and ROCOF from number of cycles in the last second
            std::size_t zc_count = range_length_zc(state.zc_tail, state.zc_head);

            // We need at least two zero crossings (i.e. one cycle) for a valid frequency estimate
            if (zc_count < 2) {
                // Not enough zero crossings; retain previous estimates
                state.frequencies[_head] = state.frequencies[prev1];
                state.rocofs[_head] = state.rocofs[prev1];
            } else {
                auto cycles = (zc_count - 1) / 2;
                auto period = state.zc_timestamps[index_prev_zc(state.zc_head)]
                        - state.zc_timestamps[state.zc_tail];
                if (!(0 <= period && period <= One_Second_Period)) {
                    std::snprintf(_error, sizeof(_error),
                                  "Invalid zero-crossing period detected: %lld (ns)", period);
                    return false;
                }

                Float frequency = cycles * (1 + period / static_cast<Float>(One_Second_Period));

                Float rocof = linear_regression<Float, 3, Slope_Only>(
                        {
                                0,
                                1,
                                2,
                        },
                        {
                                state.frequencies[prev2],
                                state.frequencies[prev1],
                                frequency,
                        });

                state.frequencies[_head] = frequency;
                state.rocofs[_head] = rocof;
            }
        }

        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            const Per_Channel_State &state = _channel_state[channel];
            _estimation.phasors[channel] = state.phasors[_head];
            _estimation.frequencies[channel] = state.frequencies[_head];
            _estimation.rocofs[channel] = state.rocofs[_head];
        }
        _sample = new_sample;
        _head = index_next(_head);
        _tail = index_next(_tail);

        return true;
    }

    inline const Estimation &estimation() const { return _estimation; }
    inline const char *error() const { return _error; }

private:
    static constexpr std::size_t index_next(std::size_t i) noexcept
    {
        return wrapped_sum<std::size_t, 0, Len_Circular_Buffer>(i, 1);
    }
    static constexpr std::size_t index_prev(std::size_t i) noexcept
    {
        return wrapped_sum<std::ptrdiff_t, 0, Len_Circular_Buffer>(i, -1);
    }
    static constexpr std::size_t index_next_zc(std::size_t i) noexcept
    {
        return wrapped_sum<std::size_t, 0, Len_ZC_Circular_Buffer>(i, 1);
    }
    static constexpr std::size_t index_prev_zc(std::size_t i) noexcept
    {
        return wrapped_sum<std::ptrdiff_t, 0, Len_ZC_Circular_Buffer>(i, -1);
    }
    static constexpr std::size_t range_length_zc(std::size_t tail, std::size_t head) noexcept
    {
        return wrapped_sum<std::ptrdiff_t, 0, Len_ZC_Circular_Buffer>(head, -std::ptrdiff_t(tail));
    }

    Sample _sample = {};
    Estimation _estimation = {};
    char _error[256] = {};

    // Constants
    Complex _twiddle_factor; // Rotates phasor by 2pi/N per sample

    // Per-channel state; struct-of-arrays for cache efficiency
    struct Per_Channel_State
    {
        Sliding_MMM<Sample_Value, Len_Circular_Buffer> samples_heap;
        Complex phasors[Len_Circular_Buffer];
        Float frequencies[Len_Circular_Buffer];
        Float rocofs[Len_Circular_Buffer];
        Timestamp zc_timestamps[Len_ZC_Circular_Buffer];
        std::size_t zc_head = 0; // One past the last valid position in zc_timestamps
        std::size_t zc_tail = 0; // Oldest valid position in zc_timestamps
    };
    Per_Channel_State _channel_state[N_Channels] = {};
    std::size_t _head = 0; // One past the last valid position; next to write
    std::size_t _tail = 0; // Oldest valid position (same as (head - N) mod Len_Circular_Buffer)
};

} // namespace qpmu
