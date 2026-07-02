#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "qpmu/core.h"
#include "../interface.hpp"

namespace qpmu {

class Host_CSV_File_Reader
{
public:
    explicit Host_CSV_File_Reader(const char *csv_path);
    ~Host_CSV_File_Reader() = default;

    Host_CSV_File_Reader(const Host_CSV_File_Reader &) = delete;
    Host_CSV_File_Reader &operator=(const Host_CSV_File_Reader &) = delete;
    Host_CSV_File_Reader(Host_CSV_File_Reader &&) = default;
    Host_CSV_File_Reader &operator=(Host_CSV_File_Reader &&) = default;

    bool read_sample_frame() noexcept;
    const Sample_Frame &sample_frame() const noexcept;
    const char *error() const noexcept;

private:
    struct Recorded_Frame
    {
        Sample_Array samples;
        std::int64_t interval_ns;
    };

    std::vector<Recorded_Frame> _frames;
    std::size_t _index = 0;
    Sample_Frame _sample_frame = {};
    char _error[256] = {};

    std::chrono::steady_clock::time_point _next_time;
};

static_assert(DAQ_Reader<Host_CSV_File_Reader>);

} // namespace qpmu
