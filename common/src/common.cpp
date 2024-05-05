#include <ios>
#include <string>
#include <sstream>
#include <iomanip>

#include "qpmu/common.h"

namespace qpmu {

std::string phasor_to_string(const ComplexType &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << phasor.real() << "+-"[phasor.imag() < 0] << std::abs(phasor.imag()) << 'j';
    return ss.str();
}

std::string phasor_polar_to_string(const ComplexType &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << std::abs(phasor) << "∠" << (std::arg(phasor) * 180 / M_PI) << "°";
    return ss.str();
}

std::string to_string(const AdcSample &sample)
{
    std::stringstream ss;
    ss << "seq_no=" << sample.seq_no << ",\t";
    for (size_t i = 0; i < NumChannels; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.ch[i] << ",";
    }
    ss << "ts=" << sample.ts << ",\t";
    ss << "delta=" << sample.delta;
    return ss.str();
}

std::string AdcSample::csv_header()
{
    std::string header;
    header += "seq_no,";
    for (size_t i = 0; i < NumChannels; ++i) {
        header += "ch" + std::to_string(i) + ',';
    }
    header += "ts,delta";
    return header;
}

std::string to_csv(const AdcSample &sample)
{
    std::string str;
    str += std::to_string(sample.seq_no);
    for (size_t i = 0; i < NumChannels; ++i) {
        str += std::to_string(sample.ch[i]);
        str += ',';
    }
    str += std::to_string(sample.ts);
    str += ',';
    str += std::to_string(sample.delta);
    return str;
}

std::string to_string(const Estimations &est)
{
    std::stringstream ss;
    ss << to_string(est.adc_sample) << ",\t";
    for (size_t i = 0; i < NumChannels; ++i) {
        ss << "phasor" << i << "=" << phasor_to_string(est.phasors[i]) << "="
           << phasor_polar_to_string(est.phasors[i]) << ",\t";
    }
    ss << "freq=" << est.freq << ",\t";
    ss << "rocof=" << est.rocof;
    return ss.str();
}

std::string Estimations::csv_header()
{
    std::string header;
    header += AdcSample::csv_header() + ',';
    for (size_t i = 0; i < NumChannels; ++i) {
        header += "phasor" + std::to_string(i) + ',';
        header += "phasor" + std::to_string(i) + "_polar" + ',';
    }
    header += "freq,rocof";
    return header;
}

std::string to_csv(const Estimations &est)
{
    std::string str;
    str += to_csv(est.adc_sample);
    for (size_t i = 0; i < NumChannels; ++i) {
        str += phasor_to_string(est.phasors[i]);
        str += ',';
        str += phasor_polar_to_string(est.phasors[i]);
        str += ',';
    }
    str += std::to_string(est.freq);
    str += ',';
    str += std::to_string(est.rocof);
    return str;
}

} // namespace qpmu