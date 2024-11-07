#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <cassert>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace qpmu {

Duration epochTime(const SystemClock::time_point &timePoint)
{
    return std::chrono::duration_cast<Duration>(timePoint.time_since_epoch());
}

std::string phasorToString(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << phasor.real() << "+-"[phasor.imag() < 0] << std::abs(phasor.imag()) << 'j';
    return ss.str();
}

std::string phasorPolarToString(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << std::abs(phasor) << "∠" << (std::arg(phasor) * 180 / M_PI) << "°";
    return ss.str();
}

std::string toString(const Sample &sample)
{
    std::stringstream ss;
    ss << "seq=" << sample.seq << ",\t";
    for (uint64_t i = 0; i < CountSignals; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.channels[i] << ", ";
    }
    ss << "ts=" << sample.timestampUsec << ",\t";
    ss << "td=" << sample.timeDeltaUsec;
    return ss.str();
}

std::string toString(const Estimation &est)
{
    std::stringstream ss;
    for (uint64_t i = 0; i < CountSignals; ++i) {
        ss << "phasor_" << i << "=" << phasorPolarToString(est.phasors[i]) << ',' << '\t';
    }
    for (uint64_t i = 0; i < CountSignals; ++i) {
        ss << "freq_" << i << "=" << est.frequencies[i] << ',' << '\t';
    }
    for (uint64_t i = 0; i < CountSignals; ++i) {
        ss << "rocof_" << i << "=" << est.rocofs[i] << ',' << '\t';
    }
    ss << "sampling_rate=" << est.samplingRate;
    return ss.str();
}

std::pair<Float, Float> linearRegression(const std::vector<Float> &x, const std::vector<Float> &y)
{
    assert(x.size() == y.size());
    assert(x.size() > 0);

    Float xMean = std::accumulate(x.begin(), x.end(), (Float)(0.0)) / x.size();
    Float yMean = std::accumulate(y.begin(), y.end(), (Float)(0.0)) / y.size();

    Float numerator = 0.0;
    Float denominator = 0.0;

    for (uint64_t i = 0; i < x.size(); ++i) {
        numerator += (x[i] - xMean) * (y[i] - yMean);
        denominator += (x[i] - xMean) * (x[i] - xMean);
    }

    Float m = denominator ? numerator / denominator : 0.0;
    Float b = yMean - (m * xMean);

    return { m, b }; // Return slope (m) and intercept (b)
}

} // namespace qpmu