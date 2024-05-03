#ifndef QPMU_UTIL_H
#define QPMU_UTIL_H

#include <cstddef>
#include <string>

#include "qpmu/common.h"

namespace qpmu {

std::string to_string(const AdcSample &sample);
std::string adcsample_csv_header();
std::string to_csv(const AdcSample &sample);

std::string to_string(const Measurement &sample);
std::string measurement_csv_header();
std::string to_csv(const Measurement &sample);

} // namespace qpmu

#endif // QPMU_UTIL_H