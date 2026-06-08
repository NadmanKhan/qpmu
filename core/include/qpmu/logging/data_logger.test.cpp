#include <gtest/gtest.h>

#include "data_logger.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

namespace qpmu::testing {

// ============================================================================
// Helpers
// ============================================================================

static Measurement_Frame make_frame(std::size_t index)
{
    Measurement_Frame mf;
    mf.seq_num = index;
    mf.timestamp = static_cast<Timestamp>(index * 1'000'000);
    for (int ch = 0; ch < 6; ++ch) {
        mf.sample_array[ch] = static_cast<Sample>(index * 10 + ch);
        mf.estimate_array[ch].phasor = Complex(
                static_cast<float>(index + ch),
                static_cast<float>(index * 2 + ch));
        mf.estimate_array[ch].frequency = 50.0f + static_cast<float>(index) * 0.01f;
        mf.estimate_array[ch].rocof = static_cast<float>(ch) * 0.1f;
    }
    return mf;
}

// ============================================================================
// Test fixture
// ============================================================================

class Data_Logger_Test : public ::testing::Test
{
protected:
    char _tmp_path[256] = "/tmp/qpmu_log_test_XXXXXX";
    int _tmp_fd = -1;

    void SetUp() override
    {
        _tmp_fd = ::mkstemp(_tmp_path);
        ASSERT_GE(_tmp_fd, 0) << "mkstemp failed";
        ::close(_tmp_fd);
        _tmp_fd = -1;
        ::unlink(_tmp_path);
    }

    void TearDown() override
    {
        ::unlink(_tmp_path);
    }

    Data_Logger::Config make_config(uint64_t max_data_bytes = 128 * 128) const
    {
        Data_Logger::Config cfg;
        cfg.file_path = _tmp_path;
        cfg.max_file_size_bytes = max_data_bytes;
        cfg.sync_interval_ms = 50;
        cfg.nominal_freq = 50;
        cfg.sampling_rate = 1200;
        cfg.dsp_config.dc_alpha = 0.001f;
        cfg.dsp_config.hann_enabled = true;
        cfg.dsp_config.ipdft_enabled = true;
        cfg.dsp_config.freq_median_window = 7;
        cfg.dsp_config.rocof_median_window = 11;
        cfg.dsp_config.rocof_regression_window = 9;
        return cfg;
    }

    Log_File_Header read_header() const
    {
        Log_File_Header hdr = {};
        int fd = ::open(_tmp_path, O_RDONLY);
        if (fd >= 0) {
            ::pread(fd, &hdr, sizeof(hdr), 0);
            ::close(fd);
        }
        return hdr;
    }

    Log_Record read_record(uint64_t index) const
    {
        Log_Record rec = {};
        int fd = ::open(_tmp_path, O_RDONLY);
        if (fd >= 0) {
            off_t offset = static_cast<off_t>(sizeof(Log_File_Header)
                                              + index * sizeof(Log_Record));
            ::pread(fd, &rec, sizeof(rec), offset);
            ::close(fd);
        }
        return rec;
    }

    void run_logger_with_frames(const Data_Logger::Config &cfg, std::size_t n_frames)
    {
        SPSC_Ring<Measurement_Frame, 4096> ring;
        std::atomic<bool> running{ true };

        for (std::size_t i = 0; i < n_frames; ++i)
            ring.try_push(make_frame(i));

        std::thread t([&] {
            Data_Logger logger(cfg);
            logger.run(ring, running);
        });

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (std::chrono::steady_clock::now() < deadline) {
            auto hdr = read_header();
            if (hdr.total_written >= n_frames)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        running.store(false, std::memory_order_release);
        t.join();
    }
};

// ============================================================================
// to_log_record tests
// ============================================================================

TEST_F(Data_Logger_Test, To_Log_Record_Conversion)
{
    auto mf = make_frame(42);
    auto rec = to_log_record(mf);

    EXPECT_EQ(rec.seq_num, 42u);
    EXPECT_EQ(rec.timestamp_ns, 42 * 1'000'000);
    EXPECT_EQ(rec._reserved[0], 0);
    EXPECT_EQ(rec._reserved[1], 0);

    for (int ch = 0; ch < 6; ++ch) {
        EXPECT_EQ(rec.samples[ch], mf.sample_array[ch]) << "ch=" << ch;
        EXPECT_FLOAT_EQ(rec.estimates[ch].re, mf.estimate_array[ch].phasor.real()) << "ch=" << ch;
        EXPECT_FLOAT_EQ(rec.estimates[ch].im, mf.estimate_array[ch].phasor.imag()) << "ch=" << ch;
        EXPECT_FLOAT_EQ(rec.estimates[ch].freq, mf.estimate_array[ch].frequency) << "ch=" << ch;
        EXPECT_FLOAT_EQ(rec.estimates[ch].rocof, mf.estimate_array[ch].rocof) << "ch=" << ch;
    }
}

TEST_F(Data_Logger_Test, To_Log_Record_Complex)
{
    Measurement_Frame mf = {};
    mf.estimate_array[0].phasor = Complex(3.0f, 4.0f);
    mf.estimate_array[3].phasor = Complex(-1.5f, 2.5f);

    auto rec = to_log_record(mf);
    EXPECT_FLOAT_EQ(rec.estimates[0].re, 3.0f);
    EXPECT_FLOAT_EQ(rec.estimates[0].im, 4.0f);
    EXPECT_FLOAT_EQ(rec.estimates[3].re, -1.5f);
    EXPECT_FLOAT_EQ(rec.estimates[3].im, 2.5f);
}

// ============================================================================
// File I/O tests
// ============================================================================

TEST_F(Data_Logger_Test, Create_New_File_Header)
{
    auto cfg = make_config();
    run_logger_with_frames(cfg, 0);

    auto hdr = read_header();
    EXPECT_EQ(std::memcmp(hdr.magic, Log_Magic, 8), 0);
    EXPECT_EQ(hdr.version, Log_Version);
    EXPECT_EQ(hdr.record_size, sizeof(Log_Record));
    EXPECT_EQ(hdr.data_offset, sizeof(Log_File_Header));
    EXPECT_EQ(hdr.write_position, 0u);
    EXPECT_EQ(hdr.total_written, 0u);
    EXPECT_EQ(hdr.nominal_freq, 50u);
    EXPECT_EQ(hdr.sampling_rate, 1200u);
    EXPECT_FLOAT_EQ(hdr.dc_alpha, 0.001f);
    EXPECT_EQ(hdr.hann_enabled, 1);
    EXPECT_EQ(hdr.ipdft_enabled, 1);
    EXPECT_EQ(hdr.freq_median_window, 7);
    EXPECT_EQ(hdr.rocof_median_window, 11);
    EXPECT_EQ(hdr.rocof_regression_window, 9);
    EXPECT_GT(hdr.creation_timestamp_ns, 0);
}

TEST_F(Data_Logger_Test, Write_And_Read_Back)
{
    constexpr std::size_t N = 100;
    auto cfg = make_config(200 * sizeof(Log_Record));
    run_logger_with_frames(cfg, N);

    auto hdr = read_header();
    EXPECT_EQ(hdr.total_written, N);
    EXPECT_EQ(hdr.write_position, N);

    for (std::size_t i = 0; i < N; ++i) {
        auto rec = read_record(i);
        auto expected = to_log_record(make_frame(i));
        EXPECT_EQ(rec.seq_num, expected.seq_num) << "record " << i;
        EXPECT_EQ(rec.timestamp_ns, expected.timestamp_ns) << "record " << i;
        for (int ch = 0; ch < 6; ++ch) {
            EXPECT_EQ(rec.samples[ch], expected.samples[ch])
                    << "record " << i << " ch=" << ch;
            EXPECT_FLOAT_EQ(rec.estimates[ch].freq, expected.estimates[ch].freq)
                    << "record " << i << " ch=" << ch;
        }
    }
}

TEST_F(Data_Logger_Test, Circular_Wrap)
{
    constexpr std::size_t max_records = 50;
    constexpr std::size_t total = 80;
    auto cfg = make_config(max_records * sizeof(Log_Record));
    run_logger_with_frames(cfg, total);

    auto hdr = read_header();
    EXPECT_EQ(hdr.total_written, total);
    EXPECT_EQ(hdr.write_position, total % max_records);

    for (std::size_t slot = 0; slot < max_records; ++slot) {
        auto rec = read_record(slot);
        std::size_t expected_index;
        if (slot < hdr.write_position) {
            expected_index = total - hdr.write_position + slot;
        } else {
            expected_index = total - max_records + (slot - hdr.write_position);
        }
        auto expected = to_log_record(make_frame(expected_index));
        EXPECT_EQ(rec.seq_num, expected.seq_num) << "slot=" << slot;
    }
}

TEST_F(Data_Logger_Test, Resume_Existing)
{
    constexpr std::size_t first_batch = 20;
    constexpr std::size_t second_batch = 10;
    auto cfg = make_config(200 * sizeof(Log_Record));

    run_logger_with_frames(cfg, first_batch);

    auto hdr1 = read_header();
    EXPECT_EQ(hdr1.total_written, first_batch);
    EXPECT_EQ(hdr1.write_position, first_batch);

    {
        SPSC_Ring<Measurement_Frame, 4096> ring;
        std::atomic<bool> running{ true };

        for (std::size_t i = first_batch; i < first_batch + second_batch; ++i)
            ring.try_push(make_frame(i));

        std::thread t([&] {
            Data_Logger logger(cfg);
            logger.run(ring, running);
        });

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (std::chrono::steady_clock::now() < deadline) {
            auto hdr = read_header();
            if (hdr.total_written >= first_batch + second_batch)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        running.store(false, std::memory_order_release);
        t.join();
    }

    auto hdr2 = read_header();
    EXPECT_EQ(hdr2.total_written, first_batch + second_batch);
    EXPECT_EQ(hdr2.write_position, first_batch + second_batch);

    auto rec = read_record(first_batch);
    auto expected = to_log_record(make_frame(first_batch));
    EXPECT_EQ(rec.seq_num, expected.seq_num);
}

TEST_F(Data_Logger_Test, Resume_Rejects_Bad_Magic)
{
    {
        int fd = ::open(_tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ASSERT_GE(fd, 0);
        char garbage[4096] = {};
        std::memset(garbage, 0xFF, sizeof(garbage));
        ::write(fd, garbage, sizeof(garbage));
        ::close(fd);
    }

    auto cfg = make_config();
    SPSC_Ring<Measurement_Frame, 64> ring;
    std::atomic<bool> running{ false };

    Data_Logger logger(cfg);
    logger.run(ring, running);

    EXPECT_NE(std::strstr(logger.error(), "bad magic"), nullptr)
            << "error was: " << logger.error();
}

} // namespace qpmu::testing
