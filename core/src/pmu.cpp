#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Unix socket headers (POSIX-specific)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "qpmu/concurrency/worker.hpp"
#include "qpmu/core.h"

// Open-C37.118 headers
#include "c37118.h"
#include "c37118command.h"
#include "c37118configuration.h"
#include "c37118data.h"
#include "c37118header.h"
#include "c37118pmustation.h"

namespace qpmu {

struct Posix_Reader
{
    static constexpr auto fn = &::read;
    static constexpr bool Is_Read = true;
};
struct Posix_Writer
{
    static constexpr auto fn = &::write;
    static constexpr bool Is_Read = false;
};

struct PMU_Server_Config
{
    std::uint16_t port = 4712;
    std::uint16_t id_code = 7;
    std::uint16_t data_rate = 50; // frames per second
    std::string station_name = "PMU Core";
    std::string header_message = "PMU Core Service v1.0";
    bool use_float = false; // true = float phasors/analogs, false = integer (16-bit)
    bool use_polar = true; // true = polar coordinates, false = rectangular
    bool use_freq_dfreq = true; // true = float frequency/ROCOF, false = integer
    FREQ_NOM nominal_freq = FN_50HZ;
};

class PMU_TCP_Server
{
public:
    PMU_TCP_Server(const PMU_Server_Config &config = PMU_Server_Config{})
        : _config(config), _server_fd(-1)
    {
        // Create frames
        _cfg1_frame = new CONFIG_1_Frame();
        _cfg2_frame = new CONFIG_Frame();
        _data_frame = new DATA_Frame(_cfg2_frame);
        _hdr_frame = new HEADER_Frame(_config.header_message.c_str());

        // Set common parameters
        _cfg1_frame->IDCODE_set(_config.id_code);
        _cfg2_frame->IDCODE_set(_config.id_code);
        _data_frame->IDCODE_set(_config.id_code);

        // Set time base (nanoseconds resolution)
        _cfg1_frame->TIME_BASE_set(static_cast<unsigned long>(Time_Resolution));
        _cfg2_frame->TIME_BASE_set(static_cast<unsigned long>(Time_Resolution));

        // Set data rate (frames per second)
        _cfg1_frame->DATA_RATE_set(_config.data_rate);
        _cfg2_frame->DATA_RATE_set(_config.data_rate);

        // Initialize timestamps
        Timestamp t = 0; // Will be updated with actual data
        _cfg1_frame->SOC_set(static_cast<unsigned long>(t / Time_Resolution));
        _cfg2_frame->SOC_set(static_cast<unsigned long>(t / Time_Resolution));
        _data_frame->SOC_set(static_cast<unsigned long>(t / Time_Resolution));
        _cfg1_frame->FRACSEC_set(static_cast<unsigned long>(t % Time_Resolution));
        _cfg2_frame->FRACSEC_set(static_cast<unsigned long>(t % Time_Resolution));
        _data_frame->FRACSEC_set(static_cast<unsigned long>(t % Time_Resolution));

        // Create PMU station
        // PMU_Station(name, idcode, FREQ_TYPE, ANALOG_TYPE, PHASOR_TYPE, COORD_TYPE)
        _pmu = new PMU_Station(_config.station_name.c_str(), _config.id_code,
                               _config.use_freq_dfreq, // FREQ_TYPE (true = float)
                               _config.use_float, // ANALOG_TYPE (true = float)
                               _config.use_float, // PHASOR_TYPE (true = float)
                               _config.use_polar); // COORD_TYPE (true = polar)

        // Add phasor channels based on signal configuration
        for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
            const auto &signal = Signal_Infos[i];
            _pmu->PHASOR_add(signal.name, 1,
                             signal.type_id == Signal_Info::Type_Voltage ? VOLTAGE : CURRENT);
        }

        // Set nominal frequency
        _pmu->FNOM_set(_config.nominal_freq);
        _pmu->CFGCNT_set(1);
        _pmu->STAT_set(0);

        // Initialize phasor values
        for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
            _pmu->PHASOR_VALUE_set(Complex(0.0f, 0.0f), i);
        }
        _pmu->FREQ_set(_config.nominal_freq == FN_50HZ ? 50.0f : 60.0f);
        _pmu->DFREQ_set(0.0f);

        // Add PMU station to configuration frames
        _cfg1_frame->PMUSTATION_ADD(_pmu);
        _cfg2_frame->PMUSTATION_ADD(_pmu);
    }

    ~PMU_TCP_Server()
    {
        stop();

        // Clean up frames
        delete _cfg1_frame;
        delete _cfg2_frame;
        delete _data_frame;
        delete _hdr_frame;
        delete _pmu;
    }

    bool start()
    {
        // Create socket
        _server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_server_fd < 0) {
            log_error("Failed to create socket");
            return false;
        }

        // Set socket options for address reuse
        int opt = 1;
        if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            log_error("Failed to set socket options");
            return false;
        }

        // Initialize server address structure
        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(_config.port);

        // Bind socket to address
        if (::bind(_server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr))
            < 0) {
            log_error("Failed to bind socket to port ", _config.port);
            return false;
        }

        // Start listening for connections
        if (listen(_server_fd, 5) < 0) {
            log_error("Failed to listen on socket");
            return false;
        }

        log_info("PMU TCP Server started on port ", _config.port);

        sockaddr_in client_address = {};
        socklen_t client_len = sizeof(client_address);

        // Main accept loop for incoming connections
        while (_server_fd > 0) {
            // Block until a client connects
            int client_fd =
                    accept(_server_fd, reinterpret_cast<sockaddr *>(&client_address), &client_len);
            if (client_fd < 0) {
                log_error("Failed to accept connection");
                stop();
                return false;
            }

            // Format client address from sockaddr_in for logging
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_address.sin_addr, ip, INET_ADDRSTRLEN);
            std::string address_str = ip;
            address_str += ':';
            address_str += std::to_string(ntohs(client_address.sin_port));
            log_info("Connection established with client ", address_str, " (FD:", client_fd,
                     "), starting RX loop");

            // Start RX loop for this client
            run_rx_loop(client_fd);
        }

        return true;
    }

    void stop()
    {
        _tx_enabled = false;
        _tx_worker.join_thread();
        shutdown(_server_fd, SHUT_RDWR);
        if (_server_fd >= 0) {
            close(_server_fd);
            _server_fd = -1;
        }
    }

    void update_measurement(const Measurement_Frame &measurement)
    {

        // Compute average frequency and ROCOF across all channels
        float freq = 0.0f;
        float dfreq = 0.0f;
        for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
            freq += measurement.estimate_array[i].frequency;
            dfreq += measurement.estimate_array[i].rocof;
        }
        freq /= Signal_Infos.size();
        dfreq /= Signal_Infos.size();

        {
            // Acquire write lock to safely update shared PMU data
            const auto lock = std::lock_guard(_frame_mutex);
            _data_frame->SOC_set(
                    static_cast<unsigned long>(measurement.timestamp / Time_Resolution));
            _data_frame->FRACSEC_set(
                    static_cast<unsigned long>(measurement.timestamp % Time_Resolution));
            for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
                _pmu->PHASOR_VALUE_set(measurement.estimate_array[i].phasor, i);
            }
            _pmu->FREQ_set(freq);
            _pmu->DFREQ_set(dfreq);
        }
    }

    inline std::string info() const noexcept
    {
        const auto lock = std::lock_guard(_mutex);
        return _info;
    }

    inline std::string error() const noexcept
    {
        const auto lock = std::lock_guard(_mutex);
        return _error;
    }

private:
    void run_rx_loop(int client_fd)
    {
        while (true) {
            auto [read_buffer, read_ok] = receive_frame(client_fd);
            if (!read_ok) {
                break; // Error or connection closed
            }
            if (read_buffer[1] == A_SYNC_CMD) {
                CMD_Frame cmd;
                cmd.unpack(read_buffer.data());
                handle_command(client_fd, cmd.CMD_get());
            } else {
                log_error("Unsupported C37.118 frame type received: 0x", std::hex, read_buffer[1],
                          std::dec);
            }
        }

        log_info("Connection closed by client, stopping RX loop and TX loop (if running)");
        {
            const auto lock = std::lock_guard(_mutex);
            _tx_enabled = false;
            _tx_worker.join_thread();
        }
    }

    void run_tx_loop(int client_fd)
    {
        const auto time_gap =
                std::chrono::microseconds(static_cast<std::int64_t>(1000000.0 / _config.data_rate));
        auto next_tx_time = std::chrono::steady_clock::now() + time_gap;
        while (_tx_enabled.load(std::memory_order_relaxed)) {
            // Transmit data frame
            int bytes_sent = transmit_frame(client_fd, _data_frame);
            if (bytes_sent <= 0) {
                log_error("Error transmitting data frame, stopping transmission");
                break;
            }
            // Sleep until next transmission time
            std::this_thread::sleep_until(next_tx_time);
            next_tx_time += time_gap;
        }
    }

    void handle_command(int client_fd, std::uint16_t cmd_code)
    {
        switch (cmd_code) {
        case 0x01: // Disable Data Transmission
        {
            log_info("Received command: Disable Data Transmission");
            {
                const auto lock = std::lock_guard(_mutex);
                _tx_enabled = false;
                _tx_worker.join_thread();
            }
            break;
        }
        case 0x02: // Enable Data Transmission
        {
            log_info("Received command: Enable Data Transmission");
            {
                const auto lock = std::lock_guard(_mutex);
                _tx_enabled = true;
                if (_tx_worker.running()) {
                    log_info("Data transmission already enabled");
                    break;
                }
                // Start TX worker thread
                _tx_worker.start([this, client_fd]() { run_tx_loop(client_fd); });
            }
            break;
        }
        case 0x03: // Send Header Frame
        case 0x04: // Send Config 1 Frame
        case 0x05: // Send Config 2 Frame
        {
            auto frame_type = cmd_code == 0x03 ? "Header"
                    : cmd_code == 0x04         ? "Configuration 1"
                                               : "Configuration 2";
            log_info("Received command: Send ", frame_type, " Frame");
            auto bytes_sent = cmd_code == 0x03 ? transmit_frame(client_fd, _hdr_frame)
                    : cmd_code == 0x04         ? transmit_frame(client_fd, _cfg1_frame)
                                               : transmit_frame(client_fd, _cfg2_frame);
            if (bytes_sent <= 0) {
                log_error("Error transmitting ", frame_type, " frame");
            }
            break;
        }
        default:
            log_error("Received unknown command code: 0x", std::hex, cmd_code, std::dec);
            break;
        }
    }

    // Read until FRAMESIZE (found at offset 2 bytes, size 2 bytes) bytes are received.
    // Returns pair of (buffer, success flag). On failure, buffer may be partial.
    std::pair<std::vector<std::uint8_t>, bool> receive_frame(int client_fd)
    {
        std::vector<std::uint8_t> buffer;

        // Read SYNC and FRAMESIZE first (4 bytes)
        buffer.resize(4);
        if (process_io_fully<Posix_Reader>(client_fd, buffer.data(), 4) <= 0) {
            return { buffer, false };
        }

        // Get and validate SYNC word (first 2 bytes)
        std::uint16_t sync_code;
        std::memcpy(&sync_code, buffer.data(), sizeof(sync_code));
        sync_code = ntohs(sync_code);
        if (sync_code != A_SYNC_AA) {
            log_error("Invalid SYNC word value: Expected 0xAA, Received 0x", std::hex, sync_code,
                      std::dec);
            return { std::move(buffer), false };
        }

        // Get and validate FRAMESIZE (next 2 bytes)
        std::uint16_t frame_size;
        std::memcpy(&frame_size, buffer.data() + 2, sizeof(frame_size));
        frame_size = ntohs(frame_size);
        if (frame_size < 4) {
            log_error("Invalid FRAMESIZE word value: received ", frame_size, " must be at least 4");
            return { std::move(buffer), false };
        }

        // Read remaining bytes until full frame received
        buffer.resize(frame_size);
        if (process_io_fully<Posix_Reader>(client_fd, buffer.data() + 4, frame_size - 4) <= 0) {
            return { std::move(buffer), false };
        }

        return { std::move(buffer), true };
    }

    template <typename Frame_Type>
    int transmit_frame(int client_fd, Frame_Type *frame)
    {
        std::uint8_t *pack_buffer;
        std::uint16_t pack_size;

        {
            const auto lock = std::shared_lock(_frame_mutex);
            pack_size = frame->pack(&pack_buffer);
        }

        auto result = process_io_fully<Posix_Writer>(client_fd, pack_buffer, pack_size);

        std::free(pack_buffer);

        return result;
    }

    template <typename Posix_IO>
    int process_io_fully(int fd, std::uint8_t *buffer, std::size_t target_bytes)
    {
        std::size_t processed_bytes = 0;
        while (processed_bytes < target_bytes) {
            int result = Posix_IO::fn(fd, buffer + processed_bytes, target_bytes - processed_bytes);
            if (result <= 0) {
                auto read = Posix_IO::Is_Read ? "read" : "write";
                auto reading = Posix_IO::Is_Read ? "reading" : "writing";
                auto received = Posix_IO::Is_Read ? "received" : "sent";
                auto from = Posix_IO::Is_Read ? "from" : "to";
                if (result < 0) {
                    log_error("Error ", reading, " ", target_bytes, " bytes ", from, " socket: `",
                              received, " ", processed_bytes, " bytes, subsequent call to ", read,
                              "()` returned ", result);
                } else /* n == 0 */ {
                    log_error("Connection closed by peer while ", reading, " ", target_bytes,
                              " bytes ", from, " socket: ", "`, received ", processed_bytes,
                              " bytes");
                }
                return result; // Error or connection closed
            }
            processed_bytes += result;
        }
        return processed_bytes;
    }

    template <typename... Args>
    void log_info(const Args &...args)
    {
        std::ostringstream ss;
        (ss << ... << args);
        std::string info = ss.str();
        {
            const auto lock = std::lock_guard(_mutex);
            std::cout << info << std::endl;
            _info = info;
            _error = "";
        }
    }

    template <typename... Args>
    void log_error(const Args &...args)
    {
        std::ostringstream ss;
        (ss << ... << args);
        std::string error = ss.str();
        {
            const auto lock = std::lock_guard(_mutex);
            std::cerr << error << std::endl;
            _error = error;
            _info = "";
        }
    }

    PMU_Server_Config _config;
    int _server_fd;
    std::string _info;
    std::string _error;

    // Frame objects (shared across threads)
    CONFIG_1_Frame *_cfg1_frame = nullptr;
    CONFIG_Frame *_cfg2_frame = nullptr;
    DATA_Frame *_data_frame = nullptr;
    HEADER_Frame *_hdr_frame = nullptr;
    PMU_Station *_pmu = nullptr;

    // Thread synchronization
    mutable std::shared_mutex _frame_mutex; // Protects frame and PMU data
    mutable std::mutex _mutex;

    std::atomic<bool> _tx_enabled = false; // Data frame transmission enabled flag
    Worker _tx_worker = {}; // Data frame transmission worker
};

} // namespace qpmu
