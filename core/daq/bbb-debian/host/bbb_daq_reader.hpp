#pragma once

#include <cstddef>
#include <cstdint>

#include "qpmu/core.h"
#include "../../interface.hpp"

struct Shared_State; // forward-declare (from shared/buffer.h)

namespace qpmu {

class BBB_DAQ_Reader
{
public:
    BBB_DAQ_Reader();
    ~BBB_DAQ_Reader();

    BBB_DAQ_Reader(const BBB_DAQ_Reader &) = delete;
    BBB_DAQ_Reader &operator=(const BBB_DAQ_Reader &) = delete;
    BBB_DAQ_Reader(BBB_DAQ_Reader &&other) noexcept;
    BBB_DAQ_Reader &operator=(BBB_DAQ_Reader &&) = delete;

    bool read_sample_frame() noexcept;
    const Sample_Frame &sample_frame() const noexcept;
    const char *error() const noexcept;

private:
    void *_mapped = nullptr;
    const volatile Shared_State *_shared = nullptr;
    std::uint32_t _last_seq = 0;
    Sample_Frame _sample_frame = {};
    char _error[256] = {};
};

static_assert(DAQ_Reader<BBB_DAQ_Reader>);

} // namespace qpmu
