#ifndef ADC_SAMPLE_H
#define ADC_SAMPLE_H

#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "signal_info.h"

constexpr size_t NUM_TOKENS = 1 + NUM_SIGNALS + 2;

struct AdcSample
{
    uint64_t seq_no;
    uint64_t ch[NUM_SIGNALS];
    uint64_t ts; // in microseconds
    uint64_t delta; // in microseconds

    static inline std::string csv_header() {
        std::stringstream ss;
        ss << "seq_no,";
        for (size_t i = 0; i < NUM_SIGNALS; ++i) {
            ss << "ch" << i << ',';
        }
        ss << "ts,delta";
        return ss.str();
    }
};

std::string to_string(const AdcSample &sample);
std::string to_csv(const AdcSample &sample);

#endif // ADC_SAMPLE_H