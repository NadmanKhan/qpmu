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
    RPMsg_Reader(const std::string &device_path)
    {
        // Open RPMsg device
        _fd = open(device_path.c_str(), O_RDWR);
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

        // Kick the PRU through the RPMsg channel
        nresult = write(_fd, 0, 0);
        if (nresult < 0) {
            std::snprintf(_error, sizeof(_error),
                          "Failed to kick PRU through RPMsg channel: write returned %d", nresult);
            return false;
        }

        // Read the sample and extra bytes from the RPMsg device
        Input_Buffer buffer;
        nresult = read(_fd, &buffer, sizeof(buffer));
        if (nresult != sizeof(buffer)) {
            std::snprintf(_error, sizeof(_error),
                          "Failed to read sample from RPMsg device: read result is %d; expected to "
                          "read %zu bytes",
                          nresult, sizeof(buffer));
            return false;
        }

        // Copy input buffer to reading
        _sample_frame.timestamp = buffer.timestamp;
        for (std::size_t i = 0; i < N_Channels; ++i) {
            _sample_frame.sample_vector[i] = buffer.samples[i];
        }

        return true;
    }

    inline const char *error() const noexcept { return _error; }
    inline const Sample_Frame &sample_frame() const noexcept { return _sample_frame; }

private:
    struct Input_Buffer
    {
        Timestamp timestamp;
        std::array<Sample, N_Channels * 30> samples; // 30 samples per channel
    };

    int _fd = -1;
    Sample_Frame _sample_frame;
    char _error[256] = {};
};

} // namespace qpmu