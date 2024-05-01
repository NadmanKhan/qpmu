#include "measurement.h"


std::string to_string(const Measurement &measurement)
{
    std::stringstream ss;
#ifdef INCLUDE_SAMPLE_IN_MEASUREMENT
    ss << to_string(measurement.adcSample) << ", \t";
#endif
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << "phasor" << i << "=" << measurement.phasors[i] << " [polar("
           << std::abs(measurement.phasors[i]) << ", "
           << (std::arg(measurement.phasors[i]) * (180 / (2 * M_PI))) << ")], ";
    }
#ifdef INCLUDE_POLARS_IN_MEASUREMENT
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << "phasor" << i << "_polar=(" << measurement.phasors_polar[i].first << ", "
           << measurement.phasors_polar[i].second << "), ";
    }
#endif
    ss << "freq=" << measurement.freq << ",";
    return ss.str();
}

std::string to_csv(const Measurement &measurement)
{
    std::stringstream ss;
#ifdef INCLUDE_SAMPLE_IN_MEASUREMENT
    ss << to_csv(measurement.adcSample) << ',';
#endif
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << measurement.phasors[i].real() << ',' << measurement.phasors[i].imag() << ',';
    }
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        ss << measurement.phasors_polar[i].first << ',' << measurement.phasors_polar[i].second
           << ',';
    }
    ss << measurement.freq;
    return ss.str();
}
