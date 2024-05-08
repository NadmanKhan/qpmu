
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
    desc.add_options()("help", "produce help message")("strategy",
                                                       po::value<string>()->default_value("fft"),
                                                       "Estimation strategy to use (fft or sdft)")(
            "window", po::value<SizeType>(), "Window size to use for estimation")(
            "vscale", po::value<FloatType>()->default_value(1.0), "Voltage scale factor")(
            "voffset", po::value<FloatType>()->default_value(0.0), "Voltage offset")(
            "iscale", po::value<FloatType>()->default_value(1.0), "Current scale factor")(
            "ioffset", po::value<FloatType>()->default_value(0.0), "Current offset")(
            "iformat", po::value<string>()->default_value("b"),
            "Input format: b (binary), s (human-readable string), c (comma separated "
            "\"key=value\" "
            "pairs)")("oformat", po::value<string>()->default_value("b"),
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

    EstimationStrategy strategy = EstimationStrategy::FFT;
    if (varmap.count("strategy")) {
        if (varmap["strategy"].as<string>() == "sdft") {
            strategy = EstimationStrategy::SDFT;
        }
    }

    SizeType window_size = 0;
    if (!varmap.count("window")) {
        cerr << "Window size is required\n";
        cerr << desc << '\n';
        return 1;
    }
    window_size = varmap["window"].as<SizeType>();

    FloatType vscale = varmap["vscale"].as<FloatType>();
    FloatType voffset = varmap["voffset"].as<FloatType>();
    FloatType iscale = varmap["iscale"].as<FloatType>();
    FloatType ioffset = varmap["ioffset"].as<FloatType>();

    std::cerr << "vscale: " << vscale << ", voffset: " << voffset << ", iscale: " << iscale
              << ", ioffset: " << ioffset << '\n';

    enum Format { FormatReadableStr, FormatCsv, FormatBinary };
    Format inputFormat;
    Format outputFormat;
    if (varmap["iformat"].as<string>() == "s") {
        inputFormat = FormatReadableStr;
    } else if (varmap["iformat"].as<string>() == "c") {
        inputFormat = FormatCsv;
    } else {
        inputFormat = FormatBinary;
    }
    if (varmap["oformat"].as<string>() == "s") {
        outputFormat = FormatReadableStr;
    } else if (varmap["oformat"].as<string>() == "c") {
        outputFormat = FormatCsv;
    } else {
        outputFormat = FormatBinary;
    }

    Estimator estimator(window_size, strategy, { vscale, voffset }, { iscale, ioffset });
    Estimations measurement;
    AdcSample sample;

    auto print = [&] {
        switch (outputFormat) {
        case FormatBinary:
            std::fwrite(&measurement, sizeof(Estimations), 1, stdout);
        case FormatCsv:
            cout << to_csv(measurement) << '\n';
            break;
        default:
            cout << to_string(measurement) << '\n';
            break;
        }
    };

    if (outputFormat == FormatCsv) {
        cout << Estimations::csv_header() << '\n';
    }

    if (inputFormat == FormatReadableStr) {
        std::string line;
        while (std::getline(std::cin, line)) {
            sample = AdcSample::from_string(line);
            measurement = estimator.estimate_measurements(sample);
            print();
        }
    } else if (inputFormat == FormatBinary) {
        while (fread(&sample, sizeof(AdcSample), 1, stdin)) {
            measurement = estimator.estimate_measurements(sample);
            print();
        }
    }

    return 0;
}