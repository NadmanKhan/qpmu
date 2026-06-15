#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

#include "qpmu/core.h"
#include "../../interface.hpp"
#include "../shared/buffer.h"

namespace qpmu {

class AM335x_MCP3208_PRU_SHRAM_Reader
{
public:
    AM335x_MCP3208_PRU_SHRAM_Reader();
    ~AM335x_MCP3208_PRU_SHRAM_Reader();

    AM335x_MCP3208_PRU_SHRAM_Reader(const AM335x_MCP3208_PRU_SHRAM_Reader &) = delete;
    AM335x_MCP3208_PRU_SHRAM_Reader &operator=(const AM335x_MCP3208_PRU_SHRAM_Reader &) = delete;
    AM335x_MCP3208_PRU_SHRAM_Reader(AM335x_MCP3208_PRU_SHRAM_Reader &&other) noexcept;
    AM335x_MCP3208_PRU_SHRAM_Reader &operator=(AM335x_MCP3208_PRU_SHRAM_Reader &&) = delete;

    bool read_sample_frame() noexcept;
    const Sample_Frame &sample_frame() const noexcept;
    const char *error() const noexcept;

private:
    void *_mapped = nullptr;
    const volatile Shared_State *_shared = nullptr;
    std::uint32_t _last_seq = 0;
    Sample_Frame _sample_frame = {};
    char _error[256] = {};
    std::int64_t _utc_offset = 0;
    std::uint32_t _calibration_seq = 0;

    void calibrate_utc_offset() noexcept;
};

static_assert(DAQ_Reader<AM335x_MCP3208_PRU_SHRAM_Reader>);

} // namespace qpmu
