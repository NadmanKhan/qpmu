#ifndef QPMU_COMMON_UTIL_H
#define QPMU_COMMON_UTIL_H

#include "qpmu/defs.h"

#include <string>
#include <vector>
#include <utility>

namespace qpmu {

Duration getDuration(const SystemClock::time_point &duration);
SystemClock::time_point durationToSystemTime(const Duration &duration);
I64 durationToMicrosec(const Duration &duration);

std::string phasorToString(const Complex &phasor);
std::string phasorPolarToString(const Complex &phasor);
std::string toString(const Estimation &estimation);
std::string toString(const Sample &sample);

std::pair<Float, Float> linearRegression(const std::vector<Float> &x, const std::vector<Float> &y);

} // namespace qpmu

#endif // QPMU_COMMON_UTIL_H
