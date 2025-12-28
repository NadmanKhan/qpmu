#include <cassert>
#include <cstdio>

#include "qpmu/core.h"
#include "daq.cpp"
#include "dsp.cpp"

using namespace qpmu;

int main()
{
    // These should be configurable
    constexpr std::size_t F_Nominal = 50; // Nominal frequency in Hz
    constexpr std::size_t F_Sampling = 1200; // Sampling rate in Hz
    constexpr auto RPMsg_Device_Path = "/dev/rpmsg_pru1";

    DSP_Engine<F_Nominal, F_Sampling> dsp_engine;
    RPMsg_Sample_Reader sample_reader(RPMsg_Device_Path);

    // Service loop
    while (true) {
        // 1. Acquire new sample
        if (!sample_reader.read_sample()) {
            std::fprintf(stderr, "Error reading sample: %s\n", sample_reader.error());
            continue;
        }
        auto sample = sample_reader.sample();

        // 2. Process sample
        if (!dsp_engine.push_sample(sample)) {
            std::fprintf(stderr, "Error processing sample: %s\n", dsp_engine.error());
            continue;
        }
        auto estimation = dsp_engine.estimation();
    }
}