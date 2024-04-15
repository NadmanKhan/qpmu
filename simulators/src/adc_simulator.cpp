#include <cassert>
#include <ios>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <filesystem>

#include "adc_sample.h"

namespace fs = std::filesystem;
using std::string, std::vector, std::pair, std::cerr;

void trim(string &str)
{
    for (char space : { ' ', '\t', '\n', '\r' }) {
        str.erase(0, str.find_first_not_of(space));
        str.erase(str.find_last_not_of(space) + 1);
    }
}

int main(int argc, char *argv[])
{

    /// Check command line arguments
    /// ----------------------------

    std::stringstream usageStream;
    usageStream << "Usage: " << argv[0] << " {120,180,240}v {1,2,3}kw" << '\n';
    usageStream << "Example: " << argv[0] << " 120v 1kw" << '\n';
    usageStream << "Example: " << argv[0] << " 180v 3kw" << '\n';
    string usageStr = usageStream.str();

    if (argc != 3) {
        cerr << usageStr;
        return 1;
    }

    auto voltageStr = string(argv[1]);
    auto loadStr = string(argv[2]);
    if (voltageStr != "120v" && voltageStr != "180v" && voltageStr != "240v") {
        cerr << "Invalid voltage: " << voltageStr << '\n';
        cerr << usageStr;
        return 1;
    }
    if (loadStr != "1kw" && loadStr != "2kw" && loadStr != "3kw") {
        cerr << "Invalid load: " << loadStr << '\n';
        cerr << usageStr;
        return 1;
    }

    /// Open samples file and fill in the samples
    /// -----------------------------------------

    auto rootDir = fs::path(__FILE__).parent_path().parent_path();
    string filename = rootDir.string() + "/data/" + voltageStr + '-' + loadStr + ".csv";
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

        vector<pair<string, uint64_t>> keyValues;
        string token;
        std::stringstream lineStream(line);
        while (std::getline(lineStream, token, ',')) {
            auto pos = token.find('=');
            if (pos == string::npos) {
                break;
            }
            auto key = token.substr(0, pos);
            auto valueStr = token.substr(pos + 1);
            trim(key);
            trim(valueStr);
            if (key.empty() || valueStr.empty()) {
                break;
            }
            keyValues.push_back({ key, std::stoull(valueStr) });
        }

        if (keyValues.size() != NUM_TOKENS) {
            break;
        }

        AdcSample sample;
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

    auto lastSample = samples[0];
    for (size_t i = 1;; ++i) {
        auto sample = samples[i % samples.size()];

        sample.seq_no = i;
        sample.ts = lastSample.ts + sample.delta;

        fwrite(&sample, sizeof(AdcSample), 1, stdout);

        lastSample = sample;
    }

    return 0;
}