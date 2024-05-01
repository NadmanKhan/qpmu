#include "adc_sample.h"

std::string to_string(const AdcSample &sample) {
    std::stringstream ss;
    ss << "seq_no=" << sample.seq_no << ", \t";
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << "ch" << i << "=" << std::right << std::setw(4) << sample.ch[i] << ", ";
    }
    ss << "ts=" << sample.ts << ", \t delta=" << sample.delta << ",";
    return ss.str();
}

std::string to_csv(const AdcSample &sample) {
    std::stringstream ss;
    ss << sample.seq_no << ',';
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << sample.ch[i] << ',';
    }
    ss << sample.ts << ',' << sample.delta;
    return ss.str();
}