#include <sstream>
#include <iomanip>

#include "qpmu/common.h"

namespace qpmu::util {

std::string phasor_to_string(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << phasor.real() << "+-"[phasor.imag() < 0] << std::abs(phasor.imag()) << 'j';
    return ss.str();
}

std::string phasor_polar_to_string(const Complex &phasor)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << std::abs(phasor) << "∠" << (std::arg(phasor) * 180 / M_PI) << "°";
    return ss.str();
}

std::string to_string(const Sample &sample)
{
    std::stringstream ss;
    ss << "seq_no=" << sample.seqNo << ",\t";
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "ch" << i << "=" << std::setw(4) << sample.channels[i] << ", ";
    }
    ss << "ts=" << sample.timestampUs << ",\t";
    ss << "delta=" << sample.timeDeltaUs << ",";
    return ss.str();
}

std::string to_string(const Synchrophasor &synchrophasor)
{
    std::stringstream ss;
    ss << "timestamp_micros=" << std::to_string(synchrophasor.timestampUs) << ",\t";
    for (USize i = 0; i < CountSignals; ++i) {
        ss << "phasor_" << i << "="
           << phasor_polar_to_string(
                      std::polar(synchrophasor.magnitudes[i], synchrophasor.phaseAngles[i]))
           << ",\t";
    }
    ss << "freq=" << synchrophasor.frequency << ",\t";
    ss << "rocof=" << synchrophasor.rocof << ",";
    return ss.str();
}

std::string csv_header_for_sample()
{
    std::string header;
    for (USize i = 0; i < CountSampleFields; ++i) {
        header += SampleFieldNames[i];
        header += ',';
    }
    return header;
}

std::string csv_header_for_synchrophasor()
{
    std::string header;
    header += "timestamp_micros,";
    for (USize i = 0; i < CountSignals; ++i) {
        header += "phasor_" + std::to_string(i) + ',';
    }
    header += "freq,rocof";
    return header;
}

bool parse_as_sample(Sample &out_sample, const char *const s,
                     const SampleFieldOrderingConfig &filed_ordering)
{
    bool reading_value = false;
    USize field_index = 0;
    U64 value = 0;

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
            switch (filed_ordering[field_index]) {
            case SampleField::SequenceNumber:
                out_sample.seqNo = value;
                break;
            case SampleField::VA:
                out_sample.channels[0] = value;
                break;
            case SampleField::VB:
                out_sample.channels[1] = value;
                break;
            case SampleField::VC:
                out_sample.channels[2] = value;
                break;
            case SampleField::IA:
                out_sample.channels[3] = value;
                break;
            case SampleField::IB:
                out_sample.channels[4] = value;
                break;
            case SampleField::IC:
                out_sample.channels[5] = value;
                break;
            case SampleField::Timestamp:
                out_sample.timestampUs = value;
                break;
            case SampleField::TimeDelta:
                out_sample.timeDeltaUs = value;
                break;
            default:
                exit(1);
            }
            value = 0;
            if (!reading_value) {
                return false;
            }
            reading_value = false;
            ++field_index;
            if (c == '\n') {
                return field_index == CountSampleFields;
            }
            break;
        }
    }

    return true;
}

// std::string Sample::csv_header()
// {
//     std::string header;
//     header += "seq_no,";
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         header += "ch" + std::to_string(i) + ',';
//     }
//     header += "ts,delta";
//     return header;
// }

std::string to_csv(const Sample &sample)
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

std::string to_csv(const Synchrophasor &synchrophasor)
{
    std::string str;
    str += std::to_string(synchrophasor.timestampUs);
    for (size_t i = 0; i < CountSignals; ++i) {
        str += phasor_polar_to_string(
                std::polar(synchrophasor.magnitudes[i], synchrophasor.phaseAngles[i]));
        str += ',';
    }
    str += std::to_string(synchrophasor.frequency);
    str += ',';
    str += std::to_string(synchrophasor.rocof);
    return str;
}

// std::string to_string(const Estimation &est)
// {
//     std::stringstream ss;
//     ss << "timestamp_micro=" << std::to_string(est.timestamp_micros) << ",\t";
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         ss << "phasor_" << i << "="
//            << phasor_polar_to_string(std::polar(est.phasor_mag[i], est.phasor_ang[i])) << ",\t";
//     }
//     ss << "freq=" << est.freq << ",\t";
//     ss << "rocof=" << est.rocof << ",";
//     return ss.str();
// }

// std::string Estimation::csv_header()
// {
//     std::string header;
//     header += "timestamp_micros,";
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         header += "phasor_" + std::to_string(i) + ',';
//     }
//     header += "freq,rocof";
//     return header;
// }

// /// Does the reverse of to_string on Sample
// Sample Sample::parse_string(const std::string &s)
// {
//     Sample sample;

//     char last_char_before_equals = 0;
//     std::uint64_t value = 0;

//     char last_char = 0;
//     for (char c : s) {
//         switch (c) {
//         case '=':
//             last_char_before_equals = last_char;
//             value = 0;
//             break;
//         case ',':
//             switch (last_char_before_equals) {
//             case 'o':
//                 sample.seq_no = value;
//                 break;
//             case 's':
//                 sample.ts = value;
//                 break;
//             case 'a':
//                 sample.delta = value;
//                 break;
//             case '0':
//             case '1':
//             case '2':
//             case '3':
//             case '4':
//             case '5':
//                 sample.ch[last_char_before_equals - '0'] = value;
//                 break;
//             }
//             break;
//         default:
//             value = (value * 10) + (std::isdigit(c) * (c - '0'));
//         }
//         last_char = c;
//     }

//     return sample;
// }

// std::string to_string(const Sample &sample)
// {
//     std::stringstream ss;
//     ss << "seq_no=" << sample.seq_no << ",\t";
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         ss << "ch" << i << "=" << std::setw(4) << sample.ch[i] << ", ";
//     }
//     ss << "ts=" << sample.ts << ",\t";
//     ss << "delta=" << sample.delta << ",";
//     return ss.str();
// }

// std::string Sample::csv_header()
// {
//     std::string header;
//     header += "seq_no,";
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         header += "ch" + std::to_string(i) + ',';
//     }
//     header += "ts,delta";
//     return header;
// }

// std::string to_csv(const Sample &sample)
// {
//     std::string str;
//     str += std::to_string(sample.seq_no);
//     for (size_t i = 0; i < CountSignalChannels; ++i) {
//         str += std::to_string(sample.ch[i]);
//         str += ',';
//     }
//     str += std::to_string(sample.ts);
//     str += ',';
//     str += std::to_string(sample.delta);
//     return str;
// }

} // namespace qpmu::util