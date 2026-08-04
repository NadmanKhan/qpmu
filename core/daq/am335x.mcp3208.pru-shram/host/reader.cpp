#include "reader.hpp"

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace qpmu {

AM335x_MCP3208_PRU_SHRAM_Reader::AM335x_MCP3208_PRU_SHRAM_Reader()
{
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        throw std::runtime_error(std::string("Failed to open /dev/mem: ") + std::strerror(errno));
    }

    _mapped =
            mmap(nullptr, PRUSS_SHARED_RAM_SIZE, PROT_READ, MAP_SHARED, fd, PRUSS_SHARED_RAM_PHYS);
    close(fd);

    if (_mapped == MAP_FAILED) {
        _mapped = nullptr;
        throw std::runtime_error(std::string("Failed to mmap PRUSS shared RAM: ")
                                 + std::strerror(errno));
    }

    _shared = static_cast<const volatile Shared_State *>(_mapped);
    _last_seq = _shared->seq;
    calibrate_utc_offset();
}

/* Assumes CLOCK_REALTIME is GPS+PPS-disciplined via chrony (setup in deployment plan). */
void AM335x_MCP3208_PRU_SHRAM_Reader::calibrate_utc_offset() noexcept
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    auto sys_ns = static_cast<std::int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
    auto pru_ns = static_cast<std::int64_t>(_shared->buf[_shared->seq & 1].timestamp_ns);
    _utc_offset = sys_ns - pru_ns;
    _calibration_seq = _shared->seq;
}

AM335x_MCP3208_PRU_SHRAM_Reader::AM335x_MCP3208_PRU_SHRAM_Reader(AM335x_MCP3208_PRU_SHRAM_Reader &&other) noexcept
    : _mapped(other._mapped),
      _shared(other._shared),
      _last_seq(other._last_seq),
      _buffer(other._buffer),
      _scan_index(other._scan_index),
      _sample_frame(other._sample_frame),
      _utc_offset(other._utc_offset),
      _calibration_seq(other._calibration_seq)
{
    other._mapped = nullptr;
    other._shared = nullptr;
}

AM335x_MCP3208_PRU_SHRAM_Reader::~AM335x_MCP3208_PRU_SHRAM_Reader()
{
    if (_mapped != nullptr) {
        munmap(_mapped, PRUSS_SHARED_RAM_SIZE);
    }
}

bool AM335x_MCP3208_PRU_SHRAM_Reader::read_sample_frame() noexcept
{
    if (_shared->magic != QPMU_PRU_MAGIC) {
        std::snprintf(_error, sizeof(_error),
                      "QPMU PRU firmware not publishing shared state (magic=0x%08x)",
                      _shared->magic);
        return false;
    }

    if (_scan_index >= NUM_SCANS) {
        std::uint32_t seq;
        const auto start = std::chrono::steady_clock::now();
        const auto start_heartbeat = _shared->heartbeat;
        while ((seq = _shared->seq) == _last_seq) {
            if (std::chrono::steady_clock::now() - start >= std::chrono::seconds(2)) {
                const auto heartbeat = _shared->heartbeat;
                std::snprintf(_error, sizeof(_error),
                              "PRU not publishing frames (seq=%u, heartbeat=%u->%u, status=%u, sample=%u)",
                              _last_seq, start_heartbeat, heartbeat, _shared->status,
                              _shared->sample_index);
                return false;
            }
        }

        const volatile Sample_Buffer *buf = &_shared->buf[seq & 1];
        std::memcpy(&_buffer, (const void *)buf, sizeof(_buffer));
        _scan_index = 0;
        _last_seq = seq;

        if (seq - _calibration_seq > 1000)
            calibrate_utc_offset();
    }

    constexpr Timestamp sample_period_ns = Time_Resolution / QPMU_PRU_SAMPLE_RATE_HZ;
    _sample_frame.timestamp = static_cast<Timestamp>(_buffer.timestamp_ns) + _utc_offset
                            + static_cast<Timestamp>(_scan_index) * sample_period_ns;
    for (std::size_t ch = 0; ch < Signal_Infos.size(); ++ch) {
        _sample_frame.sample_array[ch] =
                _buffer.samples[_scan_index * NUM_CHANNELS + ch];
    }

    ++_sample_frame.seq_num;
    ++_scan_index;

    return true;
}

const Sample_Frame &AM335x_MCP3208_PRU_SHRAM_Reader::sample_frame() const noexcept
{
    return _sample_frame;
}

const char *AM335x_MCP3208_PRU_SHRAM_Reader::error() const noexcept
{
    return _error;
}

} // namespace qpmu
