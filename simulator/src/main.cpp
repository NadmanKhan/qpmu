#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/fusion/sequence.hpp>
#include <thread>

#include "qpmu/common.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;
using std::string, std::vector, std::pair, std::cerr, std::cout;

int main(int argc, char *argv[])
{
    using namespace qpmu;
    /// Parse command line arguments
    /// ----------------------------

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")("v", po::value<string>(),
                                                       "voltage in volts")("p", po::value<string>(),
                                                                           "power in watts")(
            "format", po::value<string>()->default_value("b"),
            "output format: b (binary), s (human-readable string), c (comma separated "
            "\"key=value\" "
            "pairs)")("once", "print samples only once")("sleep",
                                                         "sleep for delta time between samples");

    po::variables_map varmap;
    po::store(po::parse_command_line(argc, argv, desc), varmap);
    po::notify(varmap);

    if (varmap.count("help")) {
        cout << desc << '\n';
        return 0;
    }

    if (!varmap.count("v") || !varmap.count("p")) {
        cerr << "v and p are required arguments\n";
        cerr << desc << '\n';
        return 1;
    }

    auto voltage_str = varmap["v"].as<string>();
    auto power_str = varmap["p"].as<string>();
    auto format = varmap["format"].as<string>();
    auto once = varmap.count("once");
    const bool do_sleep = varmap.count("sleep");

    uint64_t voltage;
    {
        vector<string> tokens;
        auto first_non_digit = voltage_str.find_first_not_of("0123456789");

        auto voltage_unit = voltage_str.substr(first_non_digit);
        voltage = std::stoull(voltage_str.substr(0, first_non_digit));

        boost::to_lower(voltage_unit);
        if (voltage_unit.empty()) {
            voltage_unit = "v";
        }

        if (voltage_unit == "mv") {
            voltage /= 1000;
        } else if (voltage_unit == "kv") {
            voltage *= 1000;
        } else if (voltage_unit != "v") {
            cerr << "Invalid voltage unit: " << voltage_unit << '\n';
            return 1;
        }
    }

    uint64_t power;
    {
        vector<string> tokens;
        auto first_non_digit = power_str.find_first_not_of("0123456789");

        auto power_unit = power_str.substr(first_non_digit);
        power = std::stoull(power_str.substr(0, first_non_digit));

        boost::to_lower(power_unit);
        if (power_unit.empty()) {
            power_unit = "w";
        }

        if (power_unit == "mw") {
            power /= 1000;
        } else if (power_unit == "kw") {
            power *= 1000;
        } else if (power_unit != "w") {
            cerr << "Invalid power unit: " << power_unit << '\n';
            return 1;
        }
    }

    enum { FormatReadableStr, FormatCsv, FormatBinary } outputFormat;

    if (format == "s") {
        outputFormat = FormatReadableStr;
    } else if (format == "c") {
        outputFormat = FormatCsv;
    } else if (format == "b") {
        outputFormat = FormatBinary;
    } else {
        cerr << "Invalid format: " << format << '\n';
        return 1;
    }

    boost::to_lower(format);

    /// Open samples file and fill in the samples
    /// -----------------------------------------

    auto currfile = fs::path(__FILE__);
    auto rootdir = currfile.parent_path().parent_path();
    string filename = rootdir.string() + "/data/" + std::to_string(voltage) + "v_"
            + std::to_string(power) + "w.csv";
    if (!fs::is_regular_file(filename)) {
        cerr << "File not found: " << filename << '\n';
        return 1;
    }
    std::ifstream file(filename);
    if (!file.good() || !file.is_open()) {
        cerr << "Failed to open file: " << filename << '\n';
        return 1;
    }

    vector<AdcSample> samples;
    string line;
    while (std::getline(file, line)) {
        samples.push_back(AdcSample::from_string(line));
    }
    // while (std::getline(file, line)) {
    //     if (line.empty()) {
    //         continue;
    //     }

    //     AdcSample sample;

    //     vector<string> tokens;
    //     boost::split(tokens, line, boost::is_any_of(","));

    //     tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
    //                                 [](const string &str) { return str.empty(); }),
    //                  tokens.end());

    //     if (tokens.size() != NumTokensAdcSample) {
    //         continue;
    //     }

    //     vector<pair<string, uint64_t>> kvs;
    //     std::transform(
    //             tokens.begin(), tokens.end(), std::back_inserter(kvs), [](const string &token) {
    //                 vector<string> keyValuePair;
    //                 boost::split(keyValuePair, token, boost::is_any_of("="));
    //                 if (keyValuePair.size() != 2) {
    //                     return pair<string, uint64_t>();
    //                 }
    //                 boost::trim(keyValuePair[0]);
    //                 boost::trim(keyValuePair[1]);
    //                 return std::make_pair(keyValuePair[0],
    //                 (uint64_t)std::stoull(keyValuePair[1]));
    //             });

    //     kvs.erase(std::remove_if(kvs.begin(), kvs.end(),
    //                              [](const pair<string, uint64_t> &keyValue) {
    //                                  return keyValue.first.empty();
    //                              }),
    //               kvs.end());

    //     for (const auto &[key, value] : kvs) {
    //         if (key == "seq_no") {
    //             sample.seq_no = value;
    //         } else if (key == "ts") {
    //             sample.ts = value;
    //         } else if (key == "delta") {
    //             sample.delta = value;
    //         } else {
    //             assert(key.size() == 3);
    //             assert(key[0] == 'c' && key[1] == 'h');
    //             int channel_idx = key[2] - '0';
    //             assert(0 <= channel_idx && channel_idx < (int)NumChannels);
    //             sample.ch[channel_idx] = value;
    //         }
    //     }

    //     samples.push_back(sample);
    // }

    /// Cycle through the samples and print them
    /// ----------------------------------------

    auto print = [outputFormat](const AdcSample &sample) {
        if (outputFormat == FormatCsv) {
            cout << to_csv(sample) << '\n';
            return;
        }
        if (outputFormat == FormatReadableStr) {
            cout << to_string(sample) << '\n';
            return;
        }
        std::fwrite(&sample, sizeof(AdcSample), 1, stdout);
    };

    if (once) {
        for (const auto &sample : samples) {
            print(sample);
        }
        return 0;
    }

    auto lastSample = samples[0];
    if (outputFormat == FormatCsv) {
        cout << AdcSample::csv_header() << '\n';
    }

    for (size_t i = 1;; ++i) {
        auto sample = samples[i % samples.size()];
        sample.seq_no = i;
        sample.ts = lastSample.ts + sample.delta;

        print(sample);

        std::swap(lastSample, sample);

        if (do_sleep) {
            // sleep for the delta time
            std::this_thread::sleep_for(std::chrono::microseconds(sample.delta));
        }
    }

    return 0;
}