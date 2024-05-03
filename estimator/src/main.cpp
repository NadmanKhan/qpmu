
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/program_options.hpp>
#include <fftw3.h>
#include <sdft/sdft.h>

#include "qpmu/common.h"
#include "qpmu/util.h"
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
            "format", po::value<string>()->default_value("b"),
            "output format: b (binary), s (human-readable string), c (comma separated "
            "\"key=value\" "
            "pairs)");

    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    SizeType window_size = 0;
    if (!varmap.count("window")) {
        cerr << "Window size is required\n";
        cerr << desc << '\n';
        return 1;
    }
    window_size = varmap["window"].as<SizeType>();

    EstimationStrategy strategy = EstimationStrategy::FFT;
    if (varmap.count("strategy")) {
        if (varmap["strategy"].as<string>() == "sdft") {
            strategy = EstimationStrategy::SDFT;
        }
    }

    enum { FormatReadableStr, FormatCsv, FormatBinary } outputFormat;
    if (varmap["format"].as<string>() == "s") {
        outputFormat = FormatReadableStr;
    } else if (varmap["format"].as<string>() == "c") {
        outputFormat = FormatCsv;
    } else {
        outputFormat = FormatBinary;
    }

    Estimator estimator(window_size, strategy);
    Measurement measurement;
    AdcSample sample;

    auto print = [&] {
        if (outputFormat == FormatReadableStr) {
            cout << to_string(measurement) << '\n';
        } else if (outputFormat == FormatCsv) {
            cout << to_csv(measurement) << '\n';
        } else {
            std::fwrite(&measurement, sizeof(Measurement), 1, stdout);
        }
    };

    if (outputFormat == FormatCsv) {
        cout << measurement_csv_header() << '\n';
    }

    while (fread(&sample, sizeof(AdcSample), 1, stdin)) {
        measurement = estimator.estimate_measurement(sample);
        print();
    }

    return 0;
}