#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include "adc_sample.h"
#include "signal_info.h"
#include <cstdint>
#include <complex>
#include <string>

static size_t BUFFER_SIZE = 24;

struct Measurement
{
    using FloatType = FLOAT_TYPE;
    using ComplexType = std::complex<FLOAT_TYPE>;

#ifdef INCLUDE_SAMPLE_IN_MEASUREMENT
    AdcSample adcSample;
#endif
    ComplexType phasors[NUM_SIGNALS];
    std::pair<FloatType, FloatType> phasors_polar[NUM_SIGNALS];
    FloatType freq;

    static inline std::string csv_header()
    {
        std::stringstream ss;
        ss << AdcSample::csv_header() << ',';
        for (size_t i = 0; i < NUM_SIGNALS; ++i) {
            ss << "phasor" << i << "_real,phasor" << i << "_imag,";
        }
#ifdef INCLUDE_POLARS_IN_MEASUREMENT
        for (size_t i = 0; i < NUM_SIGNALS; ++i) {
            ss << "phasor" << i << "_magn,phasor" << i << "_phas,";
        }
#endif
        ss << "freq";
        return ss.str();
    }
};

std::string to_string(const Measurement &measurement);
std::string to_csv(const Measurement &measurement);

#endif // MEASUREMENT_H