#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace qpmu {

template <typename T, uint32_t Capacity>
class SPSC_Ring
{
    static_assert(Capacity > 0 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must be trivially copyable for lock-free operation");

    static constexpr uint32_t Mask = Capacity - 1;

public:
    SPSC_Ring() = default;

    SPSC_Ring(const SPSC_Ring &) = delete;
    SPSC_Ring &operator=(const SPSC_Ring &) = delete;

    // Producer: returns false if ring is full (frame dropped).
    bool try_push(const T &item) noexcept
    {
        const uint32_t wp = _write_pos.load(std::memory_order_relaxed);
        const uint32_t next = (wp + 1) & Mask;
        if (next == _read_pos.load(std::memory_order_acquire))
            return false;
        _buffer[wp] = item;
        _write_pos.store(next, std::memory_order_release);
        return true;
    }

    // Consumer: returns false if ring is empty.
    bool try_pop(T &item) noexcept
    {
        const uint32_t rp = _read_pos.load(std::memory_order_relaxed);
        if (rp == _write_pos.load(std::memory_order_acquire))
            return false;
        item = _buffer[rp];
        _read_pos.store((rp + 1) & Mask, std::memory_order_release);
        return true;
    }

    uint32_t size_approx() const noexcept
    {
        const uint32_t wp = _write_pos.load(std::memory_order_relaxed);
        const uint32_t rp = _read_pos.load(std::memory_order_relaxed);
        return (wp - rp) & Mask;
    }

private:
    std::array<T, Capacity> _buffer = {};
    std::atomic<uint32_t> _write_pos = 0;
    std::atomic<uint32_t> _read_pos = 0;
};

} // namespace qpmu
