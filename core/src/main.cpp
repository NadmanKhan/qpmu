#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "qpmu/concurrency/spsc_ring.hpp"
#include "qpmu/concurrency/worker.hpp"
#include "qpmu/core.h"
#include "qpmu/ipc/gui_server.hpp"
#include "qpmu/logging/data_logger.hpp"
#include "interface.hpp"
#if defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
#  include "am335x.mcp3208.pru-shram/host/reader.hpp"
#else
#  include "host.csv.file/reader.hpp"
#endif
#include "dsp.cpp"

using namespace qpmu;

struct Args
{
    std::size_t nominal_freq = 50;
    std::size_t sampling_rate = 1200;
    const char *gui_socket = "/tmp/qpmu_gui.sock";
    std::size_t gui_decimation = 120;
    bool verbose = false;
#if !defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
    const char *data_file = nullptr;
#endif
    qpmu::DSP_Config dsp_config;
    const char *log_file = nullptr;
    std::size_t log_size_mb = 8192; // 8 GB
};

void print_usage(const char *program_name)
{
    std::fprintf(
            stderr,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Options:\n"
            "  -f, --frequency FREQ    Nominal frequency in Hz (50 or 60, default: 50)\n"
            "  -s, --sampling RATE     Sampling rate in Hz (default: 1200)\n"
            "  -g, --gui-socket PATH   GUI IPC socket path (default: /tmp/qpmu_gui.sock)\n"
            "  -D, --decimation N      GUI decimation factor (default: 120)\n"
            "  -v, --verbose           Print detailed estimates to stdout\n"
#if !defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
            "  -d, --data-file PATH    CSV file to replay (required)\n"
#endif
            "\nLogging options:\n"
            "  --log-file PATH         Enable data logging to file\n"
            "  --log-size MB           Max log file size in MB (default: 8192)\n"
            "\nDSP options:\n"
            "  --dc-alpha F            DC offset removal alpha (default: 0.001, 0 to disable)\n"
            "  --no-hann               Disable Hann windowing (use rectangular window)\n"
            "  --no-ipdft              Disable IpDFT frequency correction\n"
            "  --freq-median N         Frequency median filter window (default: 7, 0 to disable)\n"
            "  --rocof-median N        ROCOF median filter window (default: 11, 0 to disable)\n"
            "  --rocof-window N        ROCOF regression window (default: 9, min 3)\n"
            "\n"
            "  -h, --help              Show this help message\n"
            "\nExamples:\n"
            "  %s                      # Use defaults (50 Hz, 1200 Hz sampling)\n"
            "  %s -f 60 -s 1800        # 60 Hz system, 1800 Hz sampling\n",
            program_name, program_name, program_name);
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
        } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            args.verbose = true;
        } else if (std::strcmp(argv[i], "--dc-alpha") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.dsp_config.dc_alpha = std::atof(argv[i]);
            if (args.dsp_config.dc_alpha < 0) {
                std::fprintf(stderr, "Error: --dc-alpha must be >= 0\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "--no-hann") == 0) {
            args.dsp_config.hann_enabled = false;
        } else if (std::strcmp(argv[i], "--no-ipdft") == 0) {
            args.dsp_config.ipdft_enabled = false;
        } else if (std::strcmp(argv[i], "--freq-median") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.dsp_config.freq_median_window = std::atoi(argv[i]);
            if (args.dsp_config.freq_median_window > 31) {
                std::fprintf(stderr, "Error: --freq-median must be 0-31\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "--rocof-median") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.dsp_config.rocof_median_window = std::atoi(argv[i]);
            if (args.dsp_config.rocof_median_window > 31) {
                std::fprintf(stderr, "Error: --rocof-median must be 0-31\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "--rocof-window") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.dsp_config.rocof_regression_window = std::atoi(argv[i]);
            if (args.dsp_config.rocof_regression_window < 3
                || args.dsp_config.rocof_regression_window > 31) {
                std::fprintf(stderr, "Error: --rocof-window must be 3-31\n");
                std::exit(1);
            }
        } else if (std::strcmp(argv[i], "--log-file") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.log_file = argv[i];
        } else if (std::strcmp(argv[i], "--log-size") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                std::exit(1);
            }
            args.log_size_mb = std::atoi(argv[i]);
            if (args.log_size_mb < 1) {
                std::fprintf(stderr, "Error: --log-size must be at least 1 MB\n");
                std::exit(1);
            }
#if !defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
        } else if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--data-file") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "Error: %s requires an argument\n", argv[i - 1]);
                print_usage(argv[0]);
                std::exit(1);
            }
            args.data_file = argv[i];
#endif
        } else {
            std::fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    if (args.dsp_config.ipdft_enabled && !args.dsp_config.hann_enabled) {
        std::fprintf(stderr, "Warning: IpDFT requires Hann windowing; disabling IpDFT\n");
        args.dsp_config.ipdft_enabled = false;
    }

    return args;
}

#if defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
using DAQ_Reader_Type = qpmu::AM335x_MCP3208_PRU_SHRAM_Reader;
#else
using DAQ_Reader_Type = qpmu::Host_CSV_File_Reader;
#endif

static SPSC_Ring<Measurement_Frame, 4096> log_ring;

namespace {

void print_verbose_sample_header()
{
    std::printf("\neach channel is raw/proc; proc = phasor magnitude; G=* means sent to GUI\n");
    std::printf("%8s%c |", "seq", 'G');
    for (const auto &signal : Signal_Infos) {
        std::printf(" %8s", signal.name);
    }
    std::printf("\n");

    std::printf("%8s%c |", "--------", '-');
    for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
        std::printf(" %8s", "--------");
    }
    std::printf("\n");
}

void print_verbose_sample_row(const Sample_Frame &sample_frame,
                              const Measurement_Frame &measurement_frame, bool published_to_gui)
{
    static bool header_printed = false;
    if (!header_printed) {
        print_verbose_sample_header();
        header_printed = true;
    }

    std::printf("%8zu%c |", sample_frame.seq_num, published_to_gui ? '*' : '.');
    for (std::size_t channel = 0; channel < Signal_Infos.size(); ++channel) {
        const auto &estimate = measurement_frame.estimate_array[channel];
        std::printf(" %4u/%3.0f",
                    static_cast<unsigned>(sample_frame.sample_array[channel]),
                    static_cast<double>(std::abs(estimate.phasor)));
    }
    std::printf("\n");
    std::fflush(stdout);
}

} // namespace

template <qpmu::DAQ_Reader Reader, std::size_t F_Nominal, std::size_t F_Sampling>
int run_service(const Args &args, Reader &&sample_reader)
{
    DSP_Engine<F_Nominal, F_Sampling> dsp_engine(args.dsp_config);

    // Start GUI IPC server in worker thread
    GUI_IPC_Config gui_config;
    gui_config.socket_path = args.gui_socket;
    GUI_IPC_Server gui_server(gui_config);
    Worker gui_server_worker;
    gui_server_worker.start([&gui_server]() {
        gui_server.start(); // Blocking accept loop
    });

    // Start data logger in worker thread
    std::atomic<bool> log_running{ true };
    Worker log_worker;
    if (args.log_file) {
        Data_Logger::Config log_config;
        log_config.file_path = args.log_file;
        log_config.max_file_size_bytes = args.log_size_mb * 1024ULL * 1024ULL;
        log_config.nominal_freq = static_cast<uint32_t>(F_Nominal);
        log_config.sampling_rate = static_cast<uint32_t>(F_Sampling);
        log_config.dsp_config = args.dsp_config;

        log_worker.start([log_config, &log_running]() {
            Data_Logger logger(log_config);
            logger.run(log_ring, log_running);
        });
    }

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

        // 2. Process sample
        if (!dsp_engine.push_sample_frame(sample_frame)) {
            if (args.verbose) {
                std::fprintf(stderr, "Error processing sample: %s\n", dsp_engine.error());
            }
            continue;
        }
        auto measurement_frame = dsp_engine.measurement_frame();
        const bool published_to_gui = sample_frame.seq_num % args.gui_decimation == 0;

        // 3. Publish to GUI with decimation
        if (published_to_gui) {
            gui_server.update_measurement(measurement_frame);
        }

        // 4. Print one diagnostic row per sample (optional)
        if (args.verbose) {
            print_verbose_sample_row(sample_frame, measurement_frame, published_to_gui);
        }

        // 5. Push to data logger (non-blocking, drops if ring full)
        if (args.log_file) {
            log_ring.try_push(measurement_frame);
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
    std::fprintf(stderr, "GUI socket:        %s\n", args.gui_socket);
    std::fprintf(stderr, "GUI decimation:    %zu (%.1f Hz output)\n", args.gui_decimation,
                 static_cast<double>(args.sampling_rate) / args.gui_decimation);
    std::fprintf(stderr, "Verbose output:    %s\n", args.verbose ? "enabled" : "disabled");
    std::fprintf(stderr, "\nDSP Configuration:\n");
    std::fprintf(stderr, "  DC removal:      %s",
                 args.dsp_config.dc_alpha > 0 ? "enabled" : "disabled");
    if (args.dsp_config.dc_alpha > 0)
        std::fprintf(stderr, " (alpha=%.4f)", args.dsp_config.dc_alpha);
    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "  Hann window:     %s\n",
                 args.dsp_config.hann_enabled ? "enabled" : "disabled");
    std::fprintf(stderr, "  IpDFT:           %s\n",
                 args.dsp_config.ipdft_enabled ? "enabled" : "disabled");
    std::fprintf(stderr, "  Freq median:     %zu\n", args.dsp_config.freq_median_window);
    std::fprintf(stderr, "  ROCOF median:    %zu\n", args.dsp_config.rocof_median_window);
    std::fprintf(stderr, "  ROCOF regr. win: %zu\n", args.dsp_config.rocof_regression_window);
    std::fprintf(stderr, "\nData Logging:\n");
    if (args.log_file) {
        double retention_hours = (double(args.log_size_mb) * 1024.0 * 1024.0)
                / (double(args.sampling_rate) * sizeof(qpmu::Log_Record) * 3600.0);
        std::fprintf(stderr, "  Log file:        %s\n", args.log_file);
        std::fprintf(stderr, "  Log size:        %zu MB (~%.1f hours retention)\n",
                     args.log_size_mb, retention_hours);
    } else {
        std::fprintf(stderr, "  Disabled (use --log-file to enable)\n");
    }
    std::fprintf(stderr, "\n");

    // Construct the DAQ reader
#if defined(QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)
    DAQ_Reader_Type reader;
#else
    if (!args.data_file) {
        std::fprintf(stderr, "Error: --data-file is required for CSV replay mode\n");
        print_usage(argv[0]);
        return 1;
    }
    DAQ_Reader_Type reader(args.data_file);
#endif

    // Runtime dispatch: DSP_Engine is templated on freq/rate, so we dispatch here
    auto dispatch = [&](auto &&r) -> int {
        if (args.nominal_freq == 50 && args.sampling_rate == 1200)
            return run_service<DAQ_Reader_Type, 50, 1200>(args, std::move(r));
        if (args.nominal_freq == 50 && args.sampling_rate == 1800)
            return run_service<DAQ_Reader_Type, 50, 1800>(args, std::move(r));
        if (args.nominal_freq == 60 && args.sampling_rate == 1200)
            return run_service<DAQ_Reader_Type, 60, 1200>(args, std::move(r));
        if (args.nominal_freq == 60 && args.sampling_rate == 1800)
            return run_service<DAQ_Reader_Type, 60, 1800>(args, std::move(r));
        if (args.nominal_freq == 60 && args.sampling_rate == 3600)
            return run_service<DAQ_Reader_Type, 60, 3600>(args, std::move(r));

        std::fprintf(stderr, "Error: Unsupported configuration (freq=%zu Hz, rate=%zu Hz)\n",
                     args.nominal_freq, args.sampling_rate);
        std::fprintf(stderr, "Supported combinations:\n");
        std::fprintf(stderr, "  50 Hz: 1200, 1800 Hz sampling\n");
        std::fprintf(stderr, "  60 Hz: 1200, 1800, 3600 Hz sampling\n");
        return 1;
    };
    return dispatch(std::move(reader));
}
