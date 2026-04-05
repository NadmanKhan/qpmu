#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "qpmu/concurrency/worker.hpp"
#include "qpmu/core.h"
#include "qpmu/ipc/gui_server.hpp"
#include "daq.cpp"
#include "dsp.cpp"

using namespace qpmu;

struct Args
{
    std::size_t nominal_freq = 50; // Hz (50 or 60)
    std::size_t sampling_rate = 1200; // Hz
    const char *device_path = "/tmp/qpmu_adc.rpmsg"; // ADC device path
    const char *gui_socket = "/tmp/qpmu_gui.sock"; // GUI IPC socket path
    std::size_t gui_decimation = 120; // Decimation factor for GUI (1200/120 = 10 Hz)
    bool verbose = false; // Print detailed estimates
    bool no_kick = false; // Disable PRU kick (for FIFO/pipe testing)
};

void print_usage(const char *program_name)
{
    std::fprintf(stderr,
                 "Usage: %s [OPTIONS]\n"
                 "\n"
                 "Options:\n"
                 "  -f, --frequency FREQ    Nominal frequency in Hz (50 or 60, default: 50)\n"
                 "  -s, --sampling RATE     Sampling rate in Hz (default: 1200)\n"
                 "  -d, --device PATH       ADC device path (default: /tmp/qpmu_adc.rpmsg)\n"
                 "  -g, --gui-socket PATH   GUI IPC socket path (default: /tmp/qpmu_gui.sock)\n"
                 "  -D, --decimation N      GUI decimation factor (default: 24)\n"
                 "      --no-kick           Disable PRU kick (for FIFO/pipe testing)\n"
                 "  -v, --verbose           Print detailed estimates to stdout\n"
                 "  -h, --help              Show this help message\n"
                 "\nExamples:\n"
                 "  %s                      # Use defaults (50 Hz, 1200 Hz sampling)\n"
                 "  %s -f 60 -s 1800        # 60 Hz system, 1800 Hz sampling\n"
                 "  %s -d /dev/rpmsg0       # Use actual RPMsg device\n",
                 program_name, program_name, program_name, program_name);
}

Args parse_args(int argc, char *argv[])
{
    Args args;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            std::exit(0);
        } else if (std::strcmp(argv[i], "-f") == 0 || std::strcmp(argv[i], "--frequency") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.nominal_freq = std::atoi(argv[i]);
            if (args.nominal_freq != 50 && args.nominal_freq != 60) {
                std::fprintf(stderr, "Error: Nominal frequency must be 50 or 60 Hz\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "-s") == 0 || std::strcmp(argv[i], "--sampling") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.sampling_rate = std::atoi(argv[i]);
            if (args.sampling_rate < 100) {
                std::fprintf(stderr, "Error: Sampling rate must be at least 100 Hz\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--device") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.device_path = argv[i];
        } else if (std::strcmp(argv[i], "-g") == 0 || std::strcmp(argv[i], "--gui-socket") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.gui_socket = argv[i];
        } else if (std::strcmp(argv[i], "-D") == 0 || std::strcmp(argv[i], "--decimation") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.gui_decimation = std::atoi(argv[i]);
            if (args.gui_decimation < 1) {
                std::fprintf(stderr, "Error: Decimation factor must be at least 1\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "--no-kick") == 0) {
            args.no_kick = true;
        } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            args.verbose = true;
        } else {
            std::fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    return args;
}

template <std::size_t F_Nominal, std::size_t F_Sampling>
int run_service(const Args &args)
{
    DSP_Engine<F_Nominal, F_Sampling> dsp_engine;
    RPMsg_Reader sample_reader(args.device_path, !args.no_kick);

    // Start GUI IPC server in worker thread
    GUI_IPC_Config gui_config;
    gui_config.socket_path = args.gui_socket;
    GUI_IPC_Server gui_server(gui_config);
    Worker gui_server_worker;
    gui_server_worker.start([&gui_server]() {
        gui_server.start(); // Blocking accept loop
    });

    // Service loop
    while (true) {
        // 1. Acquire new sample
        if (!sample_reader.read_sample_frame()) {
            if (args.verbose) {
                std::fprintf(stderr, "Error reading sample: %s\n", sample_reader.error());
            }
            continue;
        }

        auto sample_frame = sample_reader.sample_frame();

        // 2. Print raw samples (optional)
        if (args.verbose) {
            std::printf("\n========== Frame %zu ==========", sample_frame.seq_num);
            std::printf("\nSamples:");
            for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
                std::printf("\n\t%s: %d", Signal_Infos[channel].name,
                            sample_frame.sample_array[channel]);
            }
        }

        // 3. Process sample
        if (!dsp_engine.push_sample_frame(sample_frame)) {
            if (args.verbose) {
                std::fprintf(stderr, "Error processing sample: %s\n", dsp_engine.error());
            }
            continue;
        }
        auto measurement_frame = dsp_engine.measurement_frame();

        // 4. Print detailed estimates (optional)
        if (args.verbose) {
            std::printf("\nPhasor estimates:");
            for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
                auto phasor = measurement_frame.estimate_array[channel].phasor;
                std::printf("\n\t%s: (%.3f, %.3f°)", Signal_Infos[channel].name, std::abs(phasor),
                            std::arg(phasor) * (180.0 / M_PI));
            }
            std::printf("\nFrequency estimates (Hz):");
            for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
                std::printf("\n\t%s: %.3f", Signal_Infos[channel].name,
                            measurement_frame.estimate_array[channel].frequency);
            }
            std::printf("\nROCOF estimates (Hz/s):");
            for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
                std::printf("\n\t%s: %.3f", Signal_Infos[channel].name,
                            measurement_frame.estimate_array[channel].rocof);
            }
        }

        // 5. Publish to GUI with decimation
        if (sample_frame.seq_num % args.gui_decimation == 0) {
            gui_server.update_measurement(measurement_frame);

            if (args.verbose) {
                std::printf("\nPublished frame %zu to GUI\n", sample_frame.seq_num);
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);

    // Print startup configuration
    std::fprintf(stderr, "QPMU Core Service\n");
    std::fprintf(stderr, "=================\n");
    std::fprintf(stderr, "Nominal frequency: %zu Hz\n", args.nominal_freq);
    std::fprintf(stderr, "Sampling rate:     %zu Hz\n", args.sampling_rate);
    std::fprintf(stderr, "ADC device:        %s\n", args.device_path);
    std::fprintf(stderr, "GUI socket:        %s\n", args.gui_socket);
    std::fprintf(stderr, "GUI decimation:    %zu (%.1f Hz output)\n", args.gui_decimation,
                 static_cast<double>(args.sampling_rate) / args.gui_decimation);
    std::fprintf(stderr, "Verbose output:    %s\n", args.verbose ? "enabled" : "disabled");
    std::fprintf(stderr, "\n");

    // Runtime dispatch based on configuration
    // Since DSP_Engine is templated, we need to dispatch to the correct instantiation
    if (args.nominal_freq == 50 && args.sampling_rate == 1200) {
        return run_service<50, 1200>(args);
    } else if (args.nominal_freq == 50 && args.sampling_rate == 1800) {
        return run_service<50, 1800>(args);
    } else if (args.nominal_freq == 60 && args.sampling_rate == 1200) {
        return run_service<60, 1200>(args);
    } else if (args.nominal_freq == 60 && args.sampling_rate == 1800) {
        return run_service<60, 1800>(args);
    } else if (args.nominal_freq == 60 && args.sampling_rate == 3600) {
        return run_service<60, 3600>(args);
    } else {
        std::fprintf(stderr, "Error: Unsupported configuration (freq=%zu Hz, rate=%zu Hz)\n",
                     args.nominal_freq, args.sampling_rate);
        std::fprintf(stderr, "Supported combinations:\n");
        std::fprintf(stderr, "  50 Hz: 1200, 1800 Hz sampling\n");
        std::fprintf(stderr, "  60 Hz: 1200, 1800, 3600 Hz sampling\n");
        return 1;
    }
}