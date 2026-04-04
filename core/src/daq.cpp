#include <cstddef>
#include <stdexcept>

// Unix headers:
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "qpmu/core.h"

namespace qpmu {

class RPMsg_Reader
{
public:
    RPMsg_Reader(const std::string &device_path, bool enable_kick = true)
        : _device_path(device_path), _enable_kick(enable_kick)
    {
        // Open RPMsg device
        // For real RPMsg devices, open as O_RDWR to support kick
        // For FIFOs/pipes in testing, open as O_RDONLY and disable kick
        int flags = _enable_kick ? O_RDWR : O_RDONLY;
        _fd = open(device_path.c_str(), flags);
        if (_fd < 0) {
            throw std::runtime_error("Failed to open " + device_path);
        }
    }

    ~RPMsg_Reader()
    {
        if (_fd >= 0) {
            close(_fd);
        }
    }

    inline bool read_sample_frame() noexcept
    {
        int nresult;

        // Kick the PRU through the RPMsg channel (only for real RPMsg devices)
        if (_enable_kick) {
            nresult = write(_fd, 0, 0);
            if (nresult < 0) {
                std::snprintf(_error, sizeof(_error),
                              "Failed to kick PRU through RPMsg channel: write returned %d",
                              nresult);
                return false;
            }
        }

        // Read the sample and extra bytes from the RPMsg device
        Input_Buffer buffer;
        nresult = read(_fd, &buffer, sizeof(buffer));
        if (nresult < 0) {
            std::snprintf(_error, sizeof(_error),
                          "Failed to read sample from device: read returned %d", nresult);
            return false;
        } else if (nresult == 0) {
            std::snprintf(_error, sizeof(_error), "End of file reached on device");
            return false;
        } else if (nresult != sizeof(buffer)) {
            std::snprintf(_error, sizeof(_error),
                          "Read bytes mismatch: read %d bytes, expected %zu bytes", nresult,
                          sizeof(buffer));
            return false;
        }

        // Copy input buffer to reading
        _sample_frame.timestamp = buffer.timestamp;
        for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
            _sample_frame.sample_array[i] = buffer.samples[i];
        }

        // Increment sequence number (for testing without RPMsg kick)
        ++_sample_frame.seq_num;

        return true;
    }

    inline const char *error() const noexcept { return _error; }
    inline const Sample_Frame &sample_frame() const noexcept { return _sample_frame; }

private:
    struct Input_Buffer
    {
        Timestamp timestamp;
        std::array<Sample, Signal_Infos.size() * 30> samples; // 30 samples per channel
    };

    std::string _device_path;
    bool _enable_kick = true;
    int _fd = -1;
    Sample_Frame _sample_frame = {};
    char _error[256] = {};
};

} // namespace qpmu