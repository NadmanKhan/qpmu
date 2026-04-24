#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "qpmu/core.h"
#include "../interface.hpp"

namespace qpmu {

class Sim_DAQ_Reader
{
public:
    explicit Sim_DAQ_Reader(const char *csv_path);
    ~Sim_DAQ_Reader() = default;

    Sim_DAQ_Reader(const Sim_DAQ_Reader &) = delete;
    Sim_DAQ_Reader &operator=(const Sim_DAQ_Reader &) = delete;
    Sim_DAQ_Reader(Sim_DAQ_Reader &&) = default;
    Sim_DAQ_Reader &operator=(Sim_DAQ_Reader &&) = default;

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

static_assert(DAQ_Reader<Sim_DAQ_Reader>);

} // namespace qpmu
