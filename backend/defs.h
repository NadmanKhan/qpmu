#ifndef DEFS_H
#define DEFS_H

#include <complex>
#include <array>

#ifdef FFTW
#  undef FFTW
#endif
#define FFTW(symbol) fftwf_##symbol

using FloatType = float;
using ComplexType = std::complex<FloatType>;

#define TO_STD_COMPLEX(ptr) (reinterpret_cast<ComplexType *>(ptr))
#define ABS(c) sqrtf(((c)[0] * (c)[0] + (c)[1] * (c)[1]))
#define ARG(c) atan2f((c)[1], (c)[0])

static constexpr size_t n_signals = 6;
static constexpr size_t n_samples = 24;

using ADCSample = std::array<uint64_t, 9>;

struct Result
{
    uint64_t seq_no;
    float frequency;
    ComplexType phasors[6];
};

#endif // DEFS_H