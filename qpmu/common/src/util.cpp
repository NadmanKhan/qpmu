#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <cassert>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace qpmu::util {

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
    ss << "seq_no=" << sample.seqNo << ",\t";
    for (USize i = 0; i < SignalCount; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.channels[i] << ", ";
    }
    ss << "ts=" << sample.timestampUs << ",\t";
    ss << "delta=" << sample.timeDeltaUs << ",";
    return ss.str();
}

std::string toString(const Synchrophasor &synchrophasor)
{
    std::stringstream ss;
    ss << "timestamp_micros=" << std::to_string(synchrophasor.timestampUs) << ",\t";
    for (USize i = 0; i < SignalCount; ++i) {
        ss << "phasor_" << i << "="
           << phasorPolarToString(
                      std::polar(synchrophasor.magnitudes[i], synchrophasor.phaseAngles[i]))
           << ",\t";
    }
    ss << "freq=" << synchrophasor.frequency << ",\t";
    ss << "rocof=" << synchrophasor.rocof << ",";
    return ss.str();
}

std::string sampleCsvHeader()
{
    std::string header;
    header += "seq_no,";
    for (USize i = 0; i < SignalCount; ++i) {
        header += "ch" + std::to_string(i) + ',';
    }
    header += "ts,delta";
    return header;
}

std::string synchrophasorCsvHeader()
{
    std::string header;
    header += "timestamp_micros,";
    for (USize i = 0; i < SignalCount; ++i) {
        header += "phasor_" + std::to_string(i) + ',';
    }
    header += "freq,rocof";
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
    for (size_t i = 0; i < SignalCount; ++i) {
        str += std::to_string(sample.channels[i]);
        str += ',';
    }
    str += std::to_string(sample.timestampUs);
    str += ',';
    str += std::to_string(sample.timeDeltaUs);
    return str;
}

std::string toCsv(const Synchrophasor &synchrophasor)
{
    std::string str;
    str += std::to_string(synchrophasor.timestampUs);
    for (size_t i = 0; i < SignalCount; ++i) {
        str += phasorPolarToString(
                std::polar(synchrophasor.magnitudes[i], synchrophasor.phaseAngles[i]));
        str += ',';
    }
    str += std::to_string(synchrophasor.frequency);
    str += ',';
    str += std::to_string(synchrophasor.rocof);
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

} // namespace qpmu::util