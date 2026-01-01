#include <cstddef>
#include <stdexcept>

// Unix headers:
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include "qpmu/core.h"

namespace qpmu {

class RPMsg_Sample_Reader
{
public:
    RPMsg_Sample_Reader(const std::string &device_path)
    {
        // Open RPMsg device
        _fd = open(device_path.c_str(), O_RDWR);
        if (_fd < 0) {
            throw std::runtime_error("Failed to open " + device_path);
        }
    }

    ~RPMsg_Sample_Reader()
    {
        if (_fd >= 0) {
            close(_fd);
        }
    }

    inline bool read_sample() noexcept
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
        nresult = read(_fd, &_buffer, sizeof(_buffer));
        if (nresult != sizeof(_buffer)) {
            std::snprintf(_error, sizeof(_error),
                          "Failed to read sample from RPMsg device: read result is %d; expected to "
                          "read %zu bytes",
                          nresult, sizeof(_buffer));
            return false;
        }

        return true;
    }

    inline const char *error() const noexcept { return _error; }
    inline const Sample &sample() const noexcept { return _buffer.sample; }

private:
    struct Read_Buffer
    {
        Sample sample;
        char extra_bytes[sizeof(Sample::values) * 29]; // extra 29 readings per channel
    };

    int _fd = -1;
    Read_Buffer _buffer = {};
    char _error[256] = {};
};

} // namespace qpmu