
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/program_options.hpp>
#include <fftw3.h>
#include <sdft/sdft.h>

#include "qpmu/common.h"
#include "qpmu/estimator.h"

using std::string, std::cout, std::cerr;
namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    using namespace qpmu;

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
            "phasor-est", po::value<string>()->default_value("fft"),
            "Phasor estimation strategy to use: fft, sdft")(
            "freq-est", po::value<string>()->default_value("fft"),
            "Frequency estimation strategy to use: cpd (consecutive phase differences), spc "
            "(same-phase crossings), zc (zero crossings)")("window", po::value<USize>(),
                                                           "Window size to use for estimation")(
            "vscale", po::value<FloatType>()->default_value(1.0), "Voltage scale factor")(
            "voffset", po::value<FloatType>()->default_value(0.0), "Voltage offset")(
            "iscale", po::value<FloatType>()->default_value(1.0), "Current scale factor")(
            "ioffset", po::value<FloatType>()->default_value(0.0), "Current offset")(
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

    PhasorEstimationStrategy phasor_est_strategy = PhasorEstimationStrategy::FFT;
    if (varmap.count("phasor-est")) {
        if (varmap["phasor-est"].as<string>() == "sdft") {
            phasor_est_strategy = PhasorEstimationStrategy::SDFT;
        }
    }

    FrequencyEstimationStrategy freq_est_strategy =
            FrequencyEstimationStrategy::ConsecutivePhaseDifferences;
    if (varmap.count("freq-est")) {
        auto str = varmap["freq-est"].as<string>();
        if (str == "spc") {
            freq_est_strategy = FrequencyEstimationStrategy::SamePhaseCrossings;
        } else if (str == "zc") {
            freq_est_strategy = FrequencyEstimationStrategy::ZeroCrossings;
        }
    }

    USize window_size = 0;
    if (!varmap.count("window")) {
        cerr << "Window size is required\n";
        cerr << desc << '\n';
        return 1;
    }
    window_size = varmap["window"].as<USize>();

    FloatType vscale = varmap["vscale"].as<FloatType>();
    FloatType voffset = varmap["voffset"].as<FloatType>();
    FloatType iscale = varmap["iscale"].as<FloatType>();
    FloatType ioffset = varmap["ioffset"].as<FloatType>();

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

    Estimator estimator(window_size, phasor_est_strategy, freq_est_strategy, { vscale, voffset },
                        { iscale, ioffset });
    Estimation estimation;
    AdcSample sample;

    auto print = [&] {
        switch (outputFormat) {
        case FormatBinary:
            std::fwrite(&estimation, sizeof(Estimation), 1, stdout);
            break;
        case FormatCsv:
            cout << to_csv(estimation) << '\n';
            break;
        default:
            cout << to_string(estimation) << '\n';
        }
    };

    if (outputFormat == FormatCsv) {
        cout << Estimation::csv_header() << '\n';
    }

    if (inputFormat == FormatReadableStr) {
        std::string line;
        while (std::getline(std::cin, line)) {
            sample = AdcSample::parse_string(line);
            estimation = estimator.add_estimation(sample);
            print();
        }
    } else if (inputFormat == FormatBinary) {
        while (fread(&sample, sizeof(AdcSample), 1, stdin)) {
            estimation = estimator.add_estimation(sample);
            print();
        }
    }

    return 0;
}