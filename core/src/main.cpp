#include <cassert>
#include <cstdio>

#include "qpmu/core.h"
#include "daq.cpp"
#include "dsp.cpp"
#include "qpmu/utilities/modulo.hpp"

using namespace qpmu;

int main()
{
    // These should be configurable
    constexpr std::size_t F_Nominal = 50; // Nominal frequency in Hz
    constexpr std::size_t F_Sampling = 1200; // Sampling rate in Hz
    constexpr auto RPMsg_Device_Path = "tmp-adc";

    DSP_Engine<F_Nominal, F_Sampling> dsp_engine;
    RPMsg_Reader sample_reader(RPMsg_Device_Path);

    Float prev_phase[N_Channels] = {};

    // Service loop
    while (true) {
        // 1. Acquire new sample
        if (!sample_reader.read_sample_frame()) {
            std::fprintf(stderr, "Error reading sample: %s\n", sample_reader.error());
            continue;
        }
        auto sample_frame = sample_reader.sample_frame();
        std::printf("\n\n");
        // std::printf("Samples: ");
        // for (std::size_t channel = 0; channel < N_Channels; ++channel) {
        //     std::printf("%d  ", sample_frame.sample_vector[channel]);

        // }
        // std::printf("\n");

        // 2. Process sample
        if (!dsp_engine.push_sample_frame(sample_frame)) {
            std::fprintf(stderr, "Error processing sample: %s\n", dsp_engine.error());
            continue;
        }
        auto estimate_vector = dsp_engine.estimate_vector();

        //     std::printf("Phasor estimates: ");
        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            auto phasor = estimate_vector[channel].phasor;
            Float phase = std::arg(phasor) * (180.0 / M_PI);
            Float phase_diff =
                    wrapping_add(phase, -prev_phase[channel], -Float(180.0), Float(180.0));
            std::printf("%c%c: (%.3f, %.3f°)  %.3f", Signals[channel].name[0],
                        Signals[channel].name[1], std::abs(phasor), phase, phase_diff);
            prev_phase[channel] = phase;
        }
        std::printf("\n");
        std::printf("\nFrequency estimates (Hz): ");
        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            std::printf("%c%c: %.3f  ", Signals[channel].name[0], Signals[channel].name[1],
                        estimate_vector[channel].frequency);
        }
        std::printf("\n");
        std::printf("ROCOF estimates (Hz/s): ");
        for (std::size_t channel = 0; channel < N_Channels; ++channel) {
            std::printf("%c%c: %.3f  ", Signals[channel].name[0], Signals[channel].name[1],
                        estimate_vector[channel].rocof);
        }
    }

    return 0;
}