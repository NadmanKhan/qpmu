#include <cassert>
#include <complex>
#include <cstddef>

#include "qpmu/algorithms/linear_algebra.hpp"
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
    static constexpr std::size_t F_Sampling = _Fs; // Sampling frequency, Hz
    static constexpr std::size_t F_Nominal = _F0; // Nominal frequency, Hz (e.g., 50 or 60 Hz)
    static constexpr std::size_t N = _Fs / _F0; // Samples per cycle
    static constexpr std::size_t Circular_Buffer_Length = 4 * N; // Length of all circular buffers

    DSP_Engine() noexcept
    {
        // Precompute twiddle factor for Sliding DFT
        _twiddle_factor = std::polar<Float>(1.0, 2.0 * M_PI / Float(N));

        // Initialize circular buffer indices
        _head = 0;
        _tail = Circular_Buffer_Length - N;
    }

    inline bool push_sample_frame(const Sample_Frame &sample_frame) noexcept
    {

        const std::size_t prev1 = modulo_prev<std::size_t, Circular_Buffer_Length>(_head);
        const std::size_t prev2 = modulo_prev<std::size_t, Circular_Buffer_Length>(prev1);

        // Validate timestamp
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

            // Phasor estimation via Sliding DFT
            estimate.phasor = _twiddle_factor // Rotate previous phasor
                    * (channel.estimates_circbuf[prev1].phasor
                       - Float(tail_sf.sample_array[ci]) // - Outgoing sample
                       + Float(head_sf.sample_array[ci]) // + Incoming sample
                    );

            // Frequency estimation via discrete differentiation over sliding window
            {
                const Float curr_phase = std::arg(estimate.phasor);
                channel.phase_diffs_circbuf[_head] =
                        wrapping_add(curr_phase, -channel.prev_phase, -Float(M_PI), Float(M_PI));
                channel.prev_phase = curr_phase;
            }
            channel.running_total_phase_displacement += channel.phase_diffs_circbuf[_head];
            channel.running_total_phase_displacement -= channel.phase_diffs_circbuf[_tail];
            const Float cycles = channel.running_total_phase_displacement / (2.0 * M_PI);
            const Float period = Float(head_sf.timestamp - tail_sf.timestamp) / Time_Resolution;
            estimate.frequency = (cycles / period); // in Hz

            // ROCOF estimation via linear regression over the last 3 samples
            estimate.rocof = linear_regression<Float, 3, Slope_Only>(
                    {
                            Float(0),
                            Float(1),
                            Float(2),
                    },
                    {
                            channel.estimates_circbuf[prev2].frequency,
                            channel.estimates_circbuf[prev1].frequency,
                            Float(estimate.frequency),
                    });
        }

        // Copy current estimates to output vector
        for (std::size_t ci = 0; ci < Signal_Infos.size(); ++ci) {
            _measurement_frame.estimate_array[ci] = _channels[ci].estimates_circbuf[_head];
        }

        // Advance circular buffer indices
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
    // Constants
    Complex _twiddle_factor; // Rotates phasor by 2pi/N per sample

    // Output variables
    Measurement_Frame _measurement_frame = {};
    char _error[256] = {};

    // State variables
    std::size_t _head = 0; // One past the last valid position; next to write
    std::size_t _tail = 0; // Oldest valid position (same as (head - N) mod Len_Buffer)
    std::array<Sample_Frame, Circular_Buffer_Length> _sample_frames_circbuf = {};
    struct Channel_State
    {
        std::array<Estimate, Circular_Buffer_Length> estimates_circbuf = {};
        std::array<Float, Circular_Buffer_Length> phase_diffs_circbuf = {};
        Float prev_phase = 0.0;
        Float running_total_phase_displacement = 0.0;
    };
    std::array<Channel_State, Signal_Infos.size()> _channels = {};
};

} // namespace qpmu
