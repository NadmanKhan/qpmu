#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include "adc_sample.h"
#include "signal_info.h"
#include <cstdint>
#include <complex>
#include <string>

template <typename _FloatType>
struct Measurement
{
    using FloatType = _FloatType;
    using ComplexType = std::complex<FloatType>;

    AdcSample adcSample;
    ComplexType phasors[NUM_SIGNALS];
    FloatType freq;
    FloatType rocof;
};

template <typename _FloatType>
inline std::string to_string(const Measurement<_FloatType> &measurement)
{
    std::stringstream ss;
    ss << "Measurement { adcSample=" << to_string(measurement.adcSample) << ", \t";
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << "phasor" << i << "=" << measurement.phasors[i] << " [polar(" << std::abs(measurement.phasors[i]) << ", " << std::arg(measurement.phasors[i]) << ")], ";
    }
    ss << "freq=" << measurement.freq << ", \t rocof=" << measurement.rocof << " }";
    return ss.str();
}

#endif // MEASUREMENT_H