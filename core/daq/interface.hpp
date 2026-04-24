#pragma once

#include <concepts>

#include "qpmu/core.h"

namespace qpmu {

template <typename T>
concept DAQ_Reader = requires(T reader)
{
    {
        reader.read_sample_frame()
    } noexcept -> std::same_as<bool>;
    {
        reader.sample_frame()
    } noexcept -> std::convertible_to<const Sample_Frame &>;
    {
        reader.error()
    } noexcept -> std::convertible_to<const char *>;
};

} // namespace qpmu
