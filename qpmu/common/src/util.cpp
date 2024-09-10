#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <cassert>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace qpmu {

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
    ss << "seq_no=" << sample.seqNo << ',' << '\t';
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.channels[i] << ", ";
    }
    ss << "ts=" << sample.timestampUs << ',' << '\t';
    ss << "delta=" << sample.timeDeltaUs << ",";
    return ss.str();
}

std::string toString(const Estimation &est)
{
    std::stringstream ss;
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "phasor_" << i << "=" << phasorPolarToString(est.phasors[i]) << ',' << '\t';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "freq_" << i << "=" << est.frequencies[i] << ',' << '\t';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "rocof_" << i << "=" << est.rocofs[i] << ',' << '\t';
    }
    ss << "sampling_rate=" << est.samplingRate << ',';
    return ss.str();
}

std::string sampleCsvHeader()
{
    std::string header;
    header += "seq_no,";
    for (USize i = 0; i < CountSignals; ++i) {
        header += "ch" + std::to_string(i) + ',';
    }
    header += "ts,delta";
    return header;
}

std::string estimationCsvHeader()
{
    std::string header;
    header += "timestamp_micros,";
    for (USize i = 0; i < CountSignals; ++i) {
        header += "phasor_" + std::to_string(i) + ',';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        header += "freq_" + std::to_string(i) + ',';
    }
    for (USize i = 0; i < CountSignals; ++i) {
        header += "rocof_" + std::to_string(i) + ',';
    }
    header += "sampling_rate";
    return header;
}

bool parseSample(Sample &out_sample, const char *const s)
{
    U64 *field_ptr = &out_sample.seqNo;
    U64 value = 0;
    bool reading_value = false;

    for (const char *p = s; *p != '\0'; ++p) {
        char c = *p;
        switch (c) {
        case '=':
            reading_value = true;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            value = (value * 10) + reading_value * (c - '0');
            break;
        case ',':
        case '\n':
            *field_ptr++ = value;
            value = 0;

            if (!reading_value)
                return false;
            reading_value = false;
            break;
        }
    }

    return true;
}

std::string toCsv(const Sample &sample)
{
    std::string str;
    str += std::to_string(sample.seqNo);
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(sample.channels[i]);
        str += ',';
    }
    str += std::to_string(sample.timestampUs);
    str += ',';
    str += std::to_string(sample.timeDeltaUs);
    return str;
}

std::string toCsv(const Estimation &est)
{
    std::string str;
    for (size_t i = 0; i < CountSignals; ++i) {
        str += phasorPolarToString(est.phasors[i]);
        str += ',';
    }
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(est.frequencies[i]);
        str += ',';
    }
    for (size_t i = 0; i < CountSignals; ++i) {
        str += std::to_string(est.rocofs[i]);
        str += ',';
    }
    str += std::to_string(est.samplingRate);
    return str;
}

std::pair<qpmu::Float, qpmu::Float> linearRegression(const std::vector<double> &x,
                                                     const std::vector<double> &y)
{
    using namespace qpmu;
    assert(x.size() == y.size());
    assert(x.size() > 0);

    Float x_mean = std::accumulate(x.begin(), x.end(), (Float)(0.0)) / x.size();
    Float y_mean = std::accumulate(y.begin(), y.end(), (Float)(0.0)) / y.size();

    Float numerator = 0.0;
    Float denominator = 0.0;

    for (USize i = 0; i < x.size(); ++i) {
        numerator += (x[i] - x_mean) * (y[i] - y_mean);
        denominator += (x[i] - x_mean) * (x[i] - x_mean);
    }

    Float m = denominator ? numerator / denominator : 0.0;
    Float b = y_mean - (m * x_mean);

    return { m, b }; // Return slope (m) and intercept (b)
}

} // namespace qpmu