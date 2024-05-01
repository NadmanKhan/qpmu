#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/fusion/sequence.hpp>

#include "adc_sample.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;
using std::string, std::vector, std::pair, std::cerr, std::cout;

int main(int argc, char *argv[])
{
    /// Parse command line arguments
    /// ----------------------------

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("v", po::value<string>(), "voltage in volts")
            ("p", po::value<string>(), "power in watts")                               
            ("format", po::value<string>()->default_value("b"), "output format: b (binary), r (readable string), csv (comma separated \"key=value\" "
            "pairs)")
            ("once", "print samples only once");

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

    auto voltageStr = varmap["v"].as<string>();
    auto powerStr = varmap["p"].as<string>();
    auto format = varmap["format"].as<string>();
    auto once = varmap.count("once");

    uint64_t voltage;
    {
        vector<string> tokens;
        auto first_non_digit = voltageStr.find_first_not_of("0123456789");

        auto voltageUnit = voltageStr.substr(first_non_digit);
        voltage = std::stoull(voltageStr.substr(0, first_non_digit));

        boost::to_lower(voltageUnit);
        if (voltageUnit.empty()) {
            voltageUnit = "v";
        }

        if (voltageUnit == "mv") {
            voltage /= 1000;
        } else if (voltageUnit == "kv") {
            voltage *= 1000;
        } else if (voltageUnit != "v") {
            cerr << "Invalid voltage unit: " << voltageUnit << '\n';
            return 1;
        }
    }

    uint64_t power;
    {
        vector<string> tokens;
        auto first_non_digit = powerStr.find_first_not_of("0123456789");

        auto powerUnit = powerStr.substr(first_non_digit);
        power = std::stoull(powerStr.substr(0, first_non_digit));

        boost::to_lower(powerUnit);
        if (powerUnit.empty()) {
            powerUnit = "w";
        }

        if (powerUnit == "mw") {
            power /= 1000;
        } else if (powerUnit == "kw") {
            power *= 1000;
        } else if (powerUnit != "w") {
            cerr << "Invalid power unit: " << powerUnit << '\n';
            return 1;
        }
    }

    enum { FormatReadable, FormatCsv, FormatBinary } outputFormat;

    if (format == "r") {
        outputFormat = FormatReadable;
    } else if (format == "csv") {
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

    auto currFile = fs::path(__FILE__);
    auto rootDir = currFile.parent_path().parent_path();
    string filename = rootDir.string() + "/data/" + std::to_string(voltage) + "v_"
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
        if (line.empty()) {
            continue;
        }

        AdcSample sample;

        vector<string> tokens;
        boost::split(tokens, line, boost::is_any_of(","));

        tokens.erase(std::remove_if(tokens.begin(), tokens.end(),
                                    [](const string &str) { return str.empty(); }),
                     tokens.end());

        if (tokens.size() != NUM_TOKENS) {
            continue;
        }

        vector<pair<string, uint64_t>> keyValues;
        std::transform(tokens.begin(), tokens.end(), std::back_inserter(keyValues),
                       [](const string &token) {
                           vector<string> keyValuePair;
                           boost::split(keyValuePair, token, boost::is_any_of("="));
                           if (keyValuePair.size() != 2) {
                               return pair<string, uint64_t>();
                           }
                           boost::trim(keyValuePair[0]);
                           boost::trim(keyValuePair[1]);
                           return std::make_pair(keyValuePair[0],
                                                 (uint64_t)std::stoull(keyValuePair[1]));
                       });

        keyValues.erase(std::remove_if(keyValues.begin(), keyValues.end(),
                                       [](const pair<string, uint64_t> &keyValue) {
                                           return keyValue.first.empty();
                                       }),
                        keyValues.end());

        for (const auto &[key, value] : keyValues) {
            if (key == "seq_no") {
                sample.seq_no = value;
            } else if (key == "ts") {
                sample.ts = value;
            } else if (key == "delta") {
                sample.delta = value;
            } else {
                assert(key.size() == 3);
                assert(key[0] == 'c' && key[1] == 'h');
                int channel = key[2] - '0';
                assert(0 <= channel && channel < NUM_SIGNALS);
                sample.ch[channel] = value;
            }
        }

        samples.push_back(sample);
    }

    /// Cycle through the samples and print them
    /// ----------------------------------------

    auto print = [outputFormat](const AdcSample &sample) {
        if (outputFormat == FormatCsv) {
            cout << to_csv(sample) << '\n';
            return;
        }
        if (outputFormat == FormatReadable) {
            cout << to_string(sample) << '\n';
            return;
        }
        fwrite(&sample, sizeof(AdcSample), 1, stdout);
    };

    if (once) {
        for (const auto &sample : samples) {
            print(sample);
        }
        return 0;
    }

    auto lastSample = samples[0];
    for (size_t i = 1;; ++i) {
        auto sample = samples[i % samples.size()];

        sample.seq_no = i;
        sample.ts = lastSample.ts + sample.delta;

        print(sample);

        lastSample = sample;
    }

    return 0;
}