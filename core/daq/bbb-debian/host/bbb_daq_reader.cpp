#include "bbb_daq_reader.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace qpmu {

BBB_DAQ_Reader::BBB_DAQ_Reader()
{
    int fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        throw std::runtime_error(
                std::string("Failed to open /dev/mem: ") + std::strerror(errno));
    }

    _mapped = mmap(nullptr, PRUSS_SHARED_RAM_SIZE, PROT_READ, MAP_SHARED,
                   fd, PRUSS_SHARED_RAM_PHYS);
    close(fd);

    if (_mapped == MAP_FAILED) {
        _mapped = nullptr;
        throw std::runtime_error(
                std::string("Failed to mmap PRUSS shared RAM: ") + std::strerror(errno));
    }

    _shared = static_cast<const volatile Shared_State *>(_mapped);
    _last_seq = _shared->seq;
}

BBB_DAQ_Reader::BBB_DAQ_Reader(BBB_DAQ_Reader &&other) noexcept
    : _mapped(other._mapped)
    , _shared(other._shared)
    , _last_seq(other._last_seq)
    , _sample_frame(other._sample_frame)
{
    other._mapped = nullptr;
    other._shared = nullptr;
}

BBB_DAQ_Reader::~BBB_DAQ_Reader()
{
    if (_mapped != nullptr) {
        munmap(_mapped, PRUSS_SHARED_RAM_SIZE);
    }
}

bool BBB_DAQ_Reader::read_sample_frame() noexcept
{
    std::uint32_t seq;
    while ((seq = _shared->seq) == _last_seq)
        ;

    const volatile Sample_Buffer *buf = &_shared->buf[seq & 1];

    _sample_frame.timestamp = static_cast<Timestamp>(buf->timestamp_ns);
    for (std::size_t i = 0; i < Signal_Infos.size(); ++i) {
        _sample_frame.sample_array[i] = buf->samples[i];
    }

    ++_sample_frame.seq_num;
    _last_seq = seq;

    return true;
}

const Sample_Frame &BBB_DAQ_Reader::sample_frame() const noexcept
{
    return _sample_frame;
}

const char *BBB_DAQ_Reader::error() const noexcept
{
    return _error;
}

} // namespace qpmu
