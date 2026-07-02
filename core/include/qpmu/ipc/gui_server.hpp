#pragma once

#include <atomic>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <string>

// Unix domain socket headers
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "qpmu/concurrency/worker.hpp"
#include "qpmu/core.h"

namespace qpmu {

struct GUI_IPC_Config
{
    std::string socket_path = "/tmp/qpmu_gui.sock";
    bool auto_cleanup = true; // Remove stale socket on startup
};

class GUI_IPC_Server
{
public:
    GUI_IPC_Server(const GUI_IPC_Config &config = GUI_IPC_Config{}) : _config(config) {}

    ~GUI_IPC_Server() { stop(); }

    // Blocking accept loop - run in worker thread
    bool start()
    {
        // Cleanup stale socket if requested
        if (_config.auto_cleanup) {
            unlink(_config.socket_path.c_str());
        }

        // Create Unix domain socket
        _server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (_server_fd < 0) {
            set_error("Failed to create Unix domain socket");
            return false;
        }

        // Initialize server address structure
        sockaddr_un server_addr = {};
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, _config.socket_path.c_str(),
                sizeof(server_addr.sun_path) - 1);

        // Bind socket to path
        if (::bind(_server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr))
            < 0) {
            set_error("Failed to bind socket to " + _config.socket_path);
            close(_server_fd);
            _server_fd = -1;
            return false;
        }
        if (chmod(_config.socket_path.c_str(), 0666) < 0) {
            set_error("Failed to set permissions on " + _config.socket_path);
            close(_server_fd);
            _server_fd = -1;
            unlink(_config.socket_path.c_str());
            return false;
        }

        // Start listening for connections
        if (listen(_server_fd, 1) < 0) {
            set_error("Failed to listen on socket");
            close(_server_fd);
            _server_fd = -1;
            return false;
        }

        _running.store(true, std::memory_order_relaxed);
        set_info("GUI IPC server started on " + _config.socket_path);

        // Main accept loop for incoming connections
        while (_running.load(std::memory_order_relaxed)) {
            // Block until a client connects
            int client_fd = accept(_server_fd, nullptr, nullptr);
            if (client_fd < 0) {
                if (_running.load(std::memory_order_relaxed)) {
                    set_error("Failed to accept connection");
                }
                break;
            }

            set_info("GUI client connected");

            // If a previous client is connected, disconnect it
            int prev_client = _client_fd.exchange(client_fd, std::memory_order_relaxed);
            if (prev_client >= 0) {
                set_info("Disconnecting previous GUI client");
                _tx_worker.join_thread();
                close(prev_client);
            }

            // Configure socket for non-blocking writes
            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

            // Limit send buffer to prevent bloat
            int sndbuf_size = sizeof(Measurement_Frame) * 10;
            setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size));

            // Start TX worker thread for this client
            _tx_worker.start([this, client_fd]() { run_tx_loop(client_fd); });
        }

        return true;
    }

    void stop()
    {
        _running.store(false, std::memory_order_relaxed);

        // Stop TX worker
        _tx_worker.join_thread();

        // Close client socket
        int client_fd = _client_fd.exchange(-1, std::memory_order_relaxed);
        if (client_fd >= 0) {
            close(client_fd);
        }

        // Close server socket
        if (_server_fd >= 0) {
            shutdown(_server_fd, SHUT_RDWR);
            close(_server_fd);
            _server_fd = -1;
        }

        // Cleanup socket file
        if (_config.auto_cleanup) {
            unlink(_config.socket_path.c_str());
        }
    }

    // Thread-safe frame update (called from main acquisition thread)
    void update_measurement(const Measurement_Frame &frame)
    {
        const auto lock = std::lock_guard(_frame_mutex);
        _latest_frame = frame;
    }

    bool is_connected() const noexcept
    {
        return _client_fd.load(std::memory_order_relaxed) >= 0;
    }

    std::string error() const noexcept
    {
        const auto lock = std::lock_guard(_error_mutex);
        return _error;
    }

    std::string info() const noexcept
    {
        const auto lock = std::lock_guard(_error_mutex);
        return _info;
    }

private:
    void run_tx_loop(int client_fd)
    {
        while (_running.load(std::memory_order_relaxed)) {
            // Check if this client is still the active one
            if (_client_fd.load(std::memory_order_relaxed) != client_fd) {
                break; // New client connected, exit this TX loop
            }

            // Get latest frame with read lock
            Measurement_Frame frame;
            {
                const auto lock = std::shared_lock(_frame_mutex);
                frame = _latest_frame;
            }

            // Transmit frame
            if (!transmit_frame(client_fd, frame)) {
                set_error("Failed to transmit frame, client disconnected");
                break;
            }

            // Rate limiting: sleep for 20ms (50 Hz)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        // Mark client as disconnected
        int expected = client_fd;
        _client_fd.compare_exchange_strong(expected, -1, std::memory_order_relaxed);
        set_info("TX loop stopped for client");
    }

    bool transmit_frame(int client_fd, const Measurement_Frame &frame)
    {
        const char *data = reinterpret_cast<const char *>(&frame);
        std::size_t total_sent = 0;
        std::size_t frame_size = sizeof(Measurement_Frame);

        while (total_sent < frame_size) {
            ssize_t sent = ::write(client_fd, data + total_sent, frame_size - total_sent);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Non-blocking socket would block, try again
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                // Real error occurred
                return false;
            }
            if (sent == 0) {
                // Connection closed
                return false;
            }
            total_sent += sent;
        }

        return true;
    }

    void set_error(const std::string &msg) noexcept
    {
        const auto lock = std::lock_guard(_error_mutex);
        _error = msg;
    }

    void set_info(const std::string &msg) noexcept
    {
        const auto lock = std::lock_guard(_error_mutex);
        _info = msg;
    }

    // Configuration
    GUI_IPC_Config _config;

    // Socket state
    int _server_fd = -1;
    std::atomic<int> _client_fd{-1};

    // Frame synchronization
    mutable std::shared_mutex _frame_mutex;
    Measurement_Frame _latest_frame{};

    // Thread management
    std::atomic<bool> _running{false};
    Worker _tx_worker;

    // Error tracking
    mutable std::mutex _error_mutex;
    std::string _error;
    std::string _info;
};

} // namespace qpmu
