
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/program_options.hpp>

#include "qpmu/common.h"
#include "qpmu/estimator.h"

using std::string, std::cout, std::cerr;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    using namespace qpmu;
    using namespace qpmu::util;

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
            "phasor-est", po::value<string>()->default_value("fft"),
            "Phasor estimation strategy to use: fft, sdft")(
            "freq-est", po::value<string>()->default_value("fft"),
            "Frequency estimation strategy to use: pd (phase differences), spc "
            "(same-phase crossings), zc (zero crossings), tbzc (time-bound zero crossings)")(
            "window", po::value<USize>(), "Window size to use for estimation")(
            "ch0-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 0")(
            "ch0-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 0")(
            "ch1-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 1")(
            "ch1-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 1")(
            "ch2-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 2")(
            "ch2-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 2")(
            "ch3-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 3")(
            "ch3-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 3")(
            "ch4-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 4")(
            "ch4-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 4")(
            "ch5-scale", po::value<FloatType>()->default_value(1.0), "Scale factor - channel 5")(
            "ch5-offset", po::value<FloatType>()->default_value(0.0), "Offset - channel 5")(
            "infmt", po::value<string>()->default_value("b"),
            "Input format: b (binary), s (human-readable string), c (comma separated "
            "\"key=value\" "
            "pairs)")("outfmt", po::value<string>()->default_value("b"),
                      "Output format: b (binary), s (human-readable string), c (comma separated "
                      "\"key=value\" "
                      "pairs)");

    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    PhasorEstimationStrategy phasor_strategy = PhasorEstimationStrategy::FFT;
    if (varmap.count("phasor-est")) {
        if (varmap["phasor-est"].as<string>() == "sdft") {
            phasor_strategy = PhasorEstimationStrategy::SDFT;
        }
    }

    FrequencyEstimationStrategy freq_strategy = FrequencyEstimationStrategy::PhaseDifferences;
    if (varmap.count("freq-est")) {
        auto str = varmap["freq-est"].as<string>();
        if (str == "spc") {
            freq_strategy = FrequencyEstimationStrategy::SamePhaseCrossings;
        } else if (str == "zc") {
            freq_strategy = FrequencyEstimationStrategy::ZeroCrossings;
        } else if (str == "tbzc") {
            freq_strategy = FrequencyEstimationStrategy::TimeBoundZeroCrossings;
        }
    }

    USize window_size = 0;
    if (!varmap.count("window")) {
        cerr << "Window size is required\n";
        cerr << desc << '\n';
        return 1;
    }
    window_size = varmap["window"].as<USize>();

    FloatType ch0_scale = varmap["ch0-scale"].as<FloatType>();
    FloatType ch0_offset = varmap["ch0-offset"].as<FloatType>();
    FloatType ch1_scale = varmap["ch1-scale"].as<FloatType>();
    FloatType ch1_offset = varmap["ch1-offset"].as<FloatType>();
    FloatType ch2_scale = varmap["ch2-scale"].as<FloatType>();
    FloatType ch2_offset = varmap["ch2-offset"].as<FloatType>();
    FloatType ch3_scale = varmap["ch3-scale"].as<FloatType>();
    FloatType ch3_offset = varmap["ch3-offset"].as<FloatType>();
    FloatType ch4_scale = varmap["ch4-scale"].as<FloatType>();
    FloatType ch4_offset = varmap["ch4-offset"].as<FloatType>();
    FloatType ch5_scale = varmap["ch5-scale"].as<FloatType>();
    FloatType ch5_offset = varmap["ch5-offset"].as<FloatType>();

    std::array<std::pair<FloatType, FloatType>, CountSignals> adjusting_params = {
        std::make_pair(ch0_scale, ch0_offset), std::make_pair(ch1_scale, ch1_offset),
        std::make_pair(ch2_scale, ch2_offset), std::make_pair(ch3_scale, ch3_offset),
        std::make_pair(ch4_scale, ch4_offset), std::make_pair(ch5_scale, ch5_offset)
    };

    enum Format { FormatReadableStr, FormatCsv, FormatBinary };
    Format inputFormat;
    Format outputFormat;
    if (varmap["infmt"].as<string>() == "s") {
        inputFormat = FormatReadableStr;
    } else if (varmap["infmt"].as<string>() == "c") {
        inputFormat = FormatCsv;
    } else {
        inputFormat = FormatBinary;
    }
    if (varmap["outfmt"].as<string>() == "s") {
        outputFormat = FormatReadableStr;
    } else if (varmap["outfmt"].as<string>() == "c") {
        outputFormat = FormatCsv;
    } else {
        outputFormat = FormatBinary;
    }

    Estimator estimator(window_size, adjusting_params, phasor_strategy, freq_strategy);
    Synchrophasor synchrophasor;
    Sample sample;

    auto print = [&] {
        switch (outputFormat) {
        case FormatBinary:
            std::fwrite(&synchrophasor, sizeof(Synchrophasor), 1, stdout);
            break;
        case FormatCsv:
            cout << to_csv(synchrophasor) << '\n';
            break;
        default:
            cout << to_string(synchrophasor) << '\n';
        }
    };

    if (outputFormat == FormatCsv) {
        cout << csv_header_for_synchrophasor() << '\n';
    }

    if (inputFormat == FormatReadableStr) {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!util::parse_as_sample(sample, line.c_str())) {
                cerr << "Failed to parse line: " << line << '\n';
                return 1;
            }
            estimator.update_estimation(sample);
            synchrophasor = estimator.synchrophasor();
            print();
        }
    } else if (inputFormat == FormatBinary) {
        while (fread(&sample, sizeof(Sample), 1, stdin)) {
            estimator.update_estimation(sample);
            synchrophasor = estimator.synchrophasor();
            print();
        }
    }

    return 0;
}