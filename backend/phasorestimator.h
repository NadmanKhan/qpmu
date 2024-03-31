#ifndef PHASORESTIMATOR_H
#define PHASORESTIMATOR_H

#include <fftw3.h>
#include <string>
#include <cstring>
#include <cassert>

#include "defs.h"

class PhasorEstimator
{
public:
    PhasorEstimator(size_t _n_samples);

    ~PhasorEstimator();

    void set(size_t signal_idx, size_t sample_idx, FloatType v);
    FloatType get(size_t signal_idx, size_t sample_idx);

    ComplexType phasorEstimation(size_t signal_idx);

private:
    size_t n_samples;
    FFTW(complex) * in[n_signals];
    FFTW(complex) * out[n_signals];
    FFTW(plan) plan[n_signals];
};

#endif // PHASORESTIMATOR_H
