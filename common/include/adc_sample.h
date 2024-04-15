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
};

inline std::string to_string(const AdcSample &sample) {
    std::stringstream ss;
    ss << "AdcSample { seq_no=" << sample.seq_no << ", \t";
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << "ch" << i << "=" << std::right << std::setw(4) << sample.ch[i] << ", ";
    }
    ss << "ts=" << sample.ts << ", \t delta=" << sample.delta << ", }";
    return ss.str();
}

#endif // ADC_SAMPLE_H