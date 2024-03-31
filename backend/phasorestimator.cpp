#include "phasorestimator.h"
#include "defs.h"
#include <fftw3.h>
#include <iostream>

#include <iostream>
using std::cout;


PhasorEstimator::PhasorEstimator(size_t _n_samples) : n_samples(_n_samples)
{
    // allocate memory
    for (int i = 0; i < n_signals; ++i) {
        in[i] = FFTW(alloc_complex)(n_samples);
        out[i] = FFTW(alloc_complex)(n_samples);
        plan[i] = FFTW(plan_dft_1d)(n_samples, in[i], out[i], FFTW_FORWARD, FFTW_ESTIMATE);
    }
    // zero out memory
    //! NOTE: DO NOT USE `memset()` 
    for (int i = 0; i < n_signals; ++i) {
        for (int j = 0; j < n_samples; ++j) {
            in[i][j][0] = 0;
            in[i][j][1] = 0;
            out[i][j][0] = 0;
            out[i][j][1] = 0;            
        }
    }
}

PhasorEstimator::~PhasorEstimator()
{
    for (int i = 0; i < n_signals; ++i) {
        FFTW(destroy_plan)(plan[i]);
        FFTW(free)(in[i]);
        FFTW(free)(out[i]);
    }
}

void PhasorEstimator::set(size_t signal_idx, size_t sample_idx, FloatType v)
{
    assert(signal_idx < n_signals);
    assert(sample_idx < n_samples);
    in[signal_idx][sample_idx][0] = v;
}

FloatType PhasorEstimator::get(size_t signal_idx, size_t sample_idx)
{
    assert(signal_idx < n_signals);
    assert(sample_idx < n_samples);
    return in[signal_idx][sample_idx][0];
}

ComplexType PhasorEstimator::phasorEstimation(size_t signal_idx)
{
    assert(signal_idx < n_signals);
    
    // for (int i = 0; i < n_samples; ++i) {
    //     cout << in[signal_idx][i][0] << " ";
    // }

    FFTW(execute)(plan[signal_idx]);
    
    ComplexType result = 0;
    float max_mag = 0;
    for (int i = 0; i < n_samples; ++i) {
        auto p = TO_STD_COMPLEX(&out[signal_idx][i]);
        auto mag = std::abs(*p);
        if (max_mag < mag) {
            result = *p;
            max_mag = mag;
        }
        // cout << *p << " ";
    }
    // cout << '\n';
    
    return result;
}
