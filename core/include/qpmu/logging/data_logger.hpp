#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <thread>
#include <unistd.h>

#include "qpmu/concurrency/spsc_ring.hpp"
#include "qpmu/core.h"

namespace qpmu {

namespace detail {
inline int datasync(int fd) noexcept
{
#if defined(__APPLE__)
    return ::fcntl(fd, F_FULLFSYNC);
#else
    return ::fdatasync(fd);
#endif
}
} // namespace detail

// ---------------------------------------------------------------------------
// Log_Record: fixed-width, architecture-independent, 128 bytes.
// ---------------------------------------------------------------------------

struct alignas(8) Log_Record
{
    uint64_t seq_num;
    int64_t timestamp_ns;
    uint16_t samples[6];
    uint16_t _reserved[2];
    struct
    {
        float re;
        float im;
        float freq;
        float rocof;
    } estimates[6];
};
static_assert(sizeof(Log_Record) == 128);

inline Log_Record to_log_record(const Measurement_Frame &mf) noexcept
{
    Log_Record r;
    r.seq_num = static_cast<uint64_t>(mf.seq_num);
    r.timestamp_ns = mf.timestamp;
    for (int i = 0; i < 6; ++i)
        r.samples[i] = mf.sample_array[i];
    r._reserved[0] = 0;
    r._reserved[1] = 0;
    for (int i = 0; i < 6; ++i) {
        r.estimates[i].re = mf.estimate_array[i].phasor.real();
        r.estimates[i].im = mf.estimate_array[i].phasor.imag();
        r.estimates[i].freq = mf.estimate_array[i].frequency;
        r.estimates[i].rocof = mf.estimate_array[i].rocof;
    }
    return r;
}

// ---------------------------------------------------------------------------
// Log_File_Header: 4096 bytes (one filesystem block).
// ---------------------------------------------------------------------------

struct alignas(4096) Log_File_Header
{
    // Identity (16 bytes)
    uint8_t magic[8]; // "QPMULOG\0"
    uint16_t version; // 1
    uint16_t record_size; // sizeof(Log_Record) == 128
    uint32_t _pad0;

    // Geometry (16 bytes)
    uint64_t max_records;
    uint64_t data_offset; // always 4096

    // State — updated on fdatasync (24 bytes)
    uint64_t write_position; // next record index (wraps at max_records)
    uint64_t total_written; // monotonic count
    int64_t last_sync_timestamp_ns;

    // Config snapshot (40 bytes)
    uint32_t nominal_freq;
    uint32_t sampling_rate;
    int64_t creation_timestamp_ns;
    float dc_alpha;
    uint8_t hann_enabled;
    uint8_t ipdft_enabled;
    uint16_t freq_median_window;
    uint16_t rocof_median_window;
    uint16_t rocof_regression_window;
    uint8_t _reserved_cfg[12];

    // Padding to 4096
    uint8_t _pad[4096 - 16 - 16 - 24 - 40];
};
static_assert(sizeof(Log_File_Header) == 4096);

static constexpr uint8_t Log_Magic[8] = { 'Q', 'P', 'M', 'U', 'L', 'O', 'G', '\0' };
static constexpr uint16_t Log_Version = 1;

// ---------------------------------------------------------------------------
// Data_Logger
// ---------------------------------------------------------------------------

class Data_Logger
{
public:
    struct Config
    {
        const char *file_path = nullptr;
        uint64_t max_file_size_bytes = 8ULL * 1024 * 1024 * 1024; // 8 GB
        uint32_t sync_interval_ms = 5000;
        uint32_t nominal_freq = 50;
        uint32_t sampling_rate = 1200;
        DSP_Config dsp_config;
    };

    explicit Data_Logger(const Config &config) : _config(config) { }

    ~Data_Logger()
    {
        if (_fd >= 0) {
            detail::datasync(_fd);
            ::close(_fd);
        }
    }

    Data_Logger(const Data_Logger &) = delete;
    Data_Logger &operator=(const Data_Logger &) = delete;

    template <uint32_t RingCap>
    void run(SPSC_Ring<Measurement_Frame, RingCap> &ring, std::atomic<bool> &running)
    {
        if (!open_or_resume()) {
            std::fprintf(stderr, "Data logger: %s\n", _error);
            return;
        }

        std::fprintf(stderr, "Data logger: started (%s, %.1f MB, %llu record slots)\n",
                     _config.file_path,
                     double(_header.max_records * sizeof(Log_Record)) / (1024.0 * 1024.0),
                     static_cast<unsigned long long>(_header.max_records));

        auto last_sync = std::chrono::steady_clock::now();

        while (running.load(std::memory_order_relaxed)) {
            Measurement_Frame frame;
            if (ring.try_pop(frame)) {
                _batch[_batch_count++] = to_log_record(frame);
                if (_batch_count == Batch_Size)
                    flush_batch();
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            auto now = std::chrono::steady_clock::now();
            auto elapsed_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sync)
                            .count();
            if (elapsed_ms >= _config.sync_interval_ms) {
                if (_batch_count > 0)
                    flush_batch();
                sync_to_disk();
                last_sync = now;
            }
        }

        // Final flush
        if (_batch_count > 0)
            flush_batch();
        sync_to_disk();
        std::fprintf(stderr, "Data logger: stopped (total records written: %llu)\n",
                     static_cast<unsigned long long>(_header.total_written));
    }

    const char *error() const noexcept { return _error; }

private:
    static constexpr uint32_t Batch_Size = 1024;

    bool open_or_resume()
    {
        struct stat st;
        bool file_exists = (::stat(_config.file_path, &st) == 0 && st.st_size > 0);

        if (file_exists)
            return resume_existing();
        else
            return create_new();
    }

    bool create_new()
    {
        const uint64_t data_bytes = _config.max_file_size_bytes;
        const uint64_t total_bytes = sizeof(Log_File_Header) + data_bytes;

        // Check available disk space
        struct statvfs vfs;
        if (::statvfs(_config.file_path, &vfs) != 0) {
            // File doesn't exist yet — check parent directory
            char parent[4096];
            std::strncpy(parent, _config.file_path, sizeof(parent) - 1);
            parent[sizeof(parent) - 1] = '\0';
            char *slash = std::strrchr(parent, '/');
            if (slash) {
                *slash = '\0';
                if (::statvfs(parent, &vfs) != 0) {
                    std::snprintf(_error, sizeof(_error), "statvfs failed: %s",
                                  std::strerror(errno));
                    return false;
                }
            }
        }
        uint64_t avail = uint64_t(vfs.f_bavail) * uint64_t(vfs.f_bsize);
        if (avail < total_bytes + 64ULL * 1024 * 1024) {
            std::snprintf(_error, sizeof(_error),
                          "insufficient disk space: need %llu MB, have %llu MB",
                          static_cast<unsigned long long>(total_bytes / (1024 * 1024)),
                          static_cast<unsigned long long>(avail / (1024 * 1024)));
            return false;
        }

        _fd = ::open(_config.file_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (_fd < 0) {
            std::snprintf(_error, sizeof(_error), "open failed: %s", std::strerror(errno));
            return false;
        }

        // Allocate file space
#if defined(__linux__)
        if (::fallocate(_fd, 0, 0, static_cast<off_t>(total_bytes)) != 0)
#endif
        {
            if (::ftruncate(_fd, static_cast<off_t>(total_bytes)) != 0) {
                std::snprintf(_error, sizeof(_error), "ftruncate failed: %s", std::strerror(errno));
                ::close(_fd);
                _fd = -1;
                return false;
            }
        }

        // Initialize header
        std::memset(&_header, 0, sizeof(_header));
        std::memcpy(_header.magic, Log_Magic, 8);
        _header.version = Log_Version;
        _header.record_size = sizeof(Log_Record);
        _header.data_offset = sizeof(Log_File_Header);
        _header.max_records = data_bytes / sizeof(Log_Record);
        _header.write_position = 0;
        _header.total_written = 0;
        _header.last_sync_timestamp_ns = 0;

        _header.nominal_freq = _config.nominal_freq;
        _header.sampling_rate = _config.sampling_rate;
        _header.dc_alpha = _config.dsp_config.dc_alpha;
        _header.hann_enabled = _config.dsp_config.hann_enabled ? 1 : 0;
        _header.ipdft_enabled = _config.dsp_config.ipdft_enabled ? 1 : 0;
        _header.freq_median_window = static_cast<uint16_t>(_config.dsp_config.freq_median_window);
        _header.rocof_median_window = static_cast<uint16_t>(_config.dsp_config.rocof_median_window);
        _header.rocof_regression_window =
                static_cast<uint16_t>(_config.dsp_config.rocof_regression_window);

        auto now = std::chrono::system_clock::now();
        _header.creation_timestamp_ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch())
                        .count();

        write_header();
        detail::datasync(_fd);
        _write_offset = _header.data_offset;
        return true;
    }

    bool resume_existing()
    {
        _fd = ::open(_config.file_path, O_RDWR, 0644);
        if (_fd < 0) {
            std::snprintf(_error, sizeof(_error), "open failed: %s", std::strerror(errno));
            return false;
        }

        ssize_t n = ::pread(_fd, &_header, sizeof(_header), 0);
        if (n != sizeof(_header)) {
            std::snprintf(_error, sizeof(_error), "header read failed");
            ::close(_fd);
            _fd = -1;
            return false;
        }

        if (std::memcmp(_header.magic, Log_Magic, 8) != 0) {
            std::snprintf(_error, sizeof(_error), "bad magic in log file");
            ::close(_fd);
            _fd = -1;
            return false;
        }

        if (_header.version != Log_Version || _header.record_size != sizeof(Log_Record)) {
            std::snprintf(_error, sizeof(_error), "incompatible log version or record size");
            ::close(_fd);
            _fd = -1;
            return false;
        }

        _write_offset = _header.data_offset + (_header.write_position * sizeof(Log_Record));

        std::fprintf(stderr, "Data logger: resuming at position %llu (total written: %llu)\n",
                     static_cast<unsigned long long>(_header.write_position),
                     static_cast<unsigned long long>(_header.total_written));
        return true;
    }

    void flush_batch()
    {
        if (_batch_count == 0)
            return;

        const uint64_t data_end = _header.data_offset + (_header.max_records * sizeof(Log_Record));
        const uint64_t batch_bytes = _batch_count * sizeof(Log_Record);

        if (_write_offset + batch_bytes <= data_end) {
            ::pwrite(_fd, _batch.data(), batch_bytes, static_cast<off_t>(_write_offset));
            _write_offset += batch_bytes;
        } else {
            // Wrap: split into two writes
            uint64_t first_bytes = data_end - _write_offset;
            if (first_bytes > 0) {
                ::pwrite(_fd, _batch.data(), first_bytes, static_cast<off_t>(_write_offset));
            }

            uint64_t remaining_bytes = batch_bytes - first_bytes;
            ::pwrite(_fd, reinterpret_cast<const char *>(_batch.data()) + first_bytes,
                     remaining_bytes, static_cast<off_t>(_header.data_offset));

            _write_offset = _header.data_offset + remaining_bytes;
        }

        _header.write_position = (_header.write_position + _batch_count) % _header.max_records;
        _header.total_written += _batch_count;
        _batch_count = 0;

        // Wrap offset if at exact boundary
        if (_write_offset >= data_end)
            _write_offset = _header.data_offset;
    }

    void sync_to_disk()
    {
        auto now = std::chrono::system_clock::now();
        _header.last_sync_timestamp_ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch())
                        .count();

        write_header();
        detail::datasync(_fd);
    }

    void write_header() { ::pwrite(_fd, &_header, sizeof(_header), 0); }

    Config _config;
    int _fd = -1;
    Log_File_Header _header = {};
    uint64_t _write_offset = 0;
    char _error[256] = {};

    std::array<Log_Record, Batch_Size> _batch = {};
    uint32_t _batch_count = 0;
};

} // namespace qpmu
