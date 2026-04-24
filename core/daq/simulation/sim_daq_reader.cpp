#include "sim_daq_reader.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace qpmu {

static bool parse_csv_line(const std::string &line, Timestamp &ts, Sample_Array &samples)
{
    // Format: timestamp_nsec,channel_0,channel_1,...,channel_5
    const char *p = line.c_str();
    char *end;

    ts = static_cast<Timestamp>(std::strtoll(p, &end, 10));
    if (*end != ',')
        return false;

    for (std::size_t i = 0; i < samples.size(); ++i) {
        p = end + 1;
        long val = std::strtol(p, &end, 10);
        samples[i] = static_cast<Sample>(val);
        if (i < samples.size() - 1 && *end != ',')
            return false;
    }
    return true;
}

Sim_DAQ_Reader::Sim_DAQ_Reader(const char *csv_path)
{
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        throw std::runtime_error(
                std::string("Failed to open CSV file: ") + csv_path);
    }

    std::string line;
    std::getline(file, line); // skip header

    struct Raw_Row
    {
        Timestamp timestamp;
        Sample_Array samples;
    };

    std::vector<Raw_Row> rows;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        Raw_Row row;
        if (!parse_csv_line(line, row.timestamp, row.samples)) {
            throw std::runtime_error(
                    std::string("Malformed CSV line: ") + line);
        }
        rows.push_back(row);
    }

    if (rows.size() < 2) {
        throw std::runtime_error("CSV file must contain at least 2 data rows");
    }

    std::int64_t total_interval = rows.back().timestamp - rows.front().timestamp;
    std::int64_t avg_interval = total_interval / static_cast<std::int64_t>(rows.size() - 1);

    _frames.reserve(rows.size());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        std::int64_t interval;
        if (i + 1 < rows.size()) {
            interval = rows[i + 1].timestamp - rows[i].timestamp;
        } else {
            interval = avg_interval;
        }
        _frames.push_back({ rows[i].samples, interval });
    }

    _next_time = std::chrono::steady_clock::now();
}

bool Sim_DAQ_Reader::read_sample_frame() noexcept
{
    std::this_thread::sleep_until(_next_time);

    const auto &frame = _frames[_index];

    auto wall_now = std::chrono::system_clock::now();
    _sample_frame.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                      wall_now.time_since_epoch())
                                      .count();
    _sample_frame.sample_array = frame.samples;
    ++_sample_frame.seq_num;

    _next_time += std::chrono::nanoseconds(frame.interval_ns);
    _index = (_index + 1) % _frames.size();

    return true;
}

const Sample_Frame &Sim_DAQ_Reader::sample_frame() const noexcept
{
    return _sample_frame;
}

const char *Sim_DAQ_Reader::error() const noexcept
{
    return _error;
}

} // namespace qpmu
