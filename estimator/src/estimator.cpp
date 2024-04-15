
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <array>

#include <sdft/sdft.h>
#include <fftw3.h>

#include "adc_sample.h"
#include "signal_info.h"
#include "measurement.h"

using std::vector, std::array, std::string, std::stringstream, std::to_string, std::complex;

enum FloatTypeChoice { FloatTypeFloat, FloatTypeDouble };

enum StrategyChoice {
    StrategySDFT, // Sliding (Window) Discrete Fourier Transform
    StrategyFFT // Fast Fourier Transform
};

template <typename FloatType>
constexpr FloatType PI = M_PI;
template <>
constexpr float PI<float> = M_PIf;
template <>
constexpr double PI<double> = M_PI;
template <>
constexpr long double PI<long double> = M_PIl;

template <typename FloatType>
constexpr FloatType DEG2RAD = PI<FloatType> / 180;
template <>
constexpr float DEG2RAD<float> = M_PIf / 180;
template <>
constexpr double DEG2RAD<double> = M_PI / 180;
template <>
constexpr long double DEG2RAD<long double> = M_PIl / 180;

template <typename FloatType>
constexpr FloatType RAD2DEG = 180 / PI<FloatType>;
template <>
constexpr float RAD2DEG<float> = 180 / M_PIf;
template <>
constexpr double RAD2DEG<double> = 180 / M_PI;
template <>
constexpr long double RAD2DEG<long double> = 180 / M_PIl;


template <typename FloatType>
void work(StrategyChoice strategy, const size_t numSamples)
{
    using ComplexType = complex<float>;
    using MeasurementType = Measurement<float>;

    vector<MeasurementType> measurements(numSamples, MeasurementType{});

#define NEXT(index) ((index != (numSamples - 1)) * (index + 1))
#define PREV(index) ((index != 0) * (index - 1) + (index == 0) * (numSamples - 1))
    size_t bufferIndex = 0;

    if (strategy == StrategyFFT) {


    } else /* (strategy == StrategySDFT) */ {
        using namespace sdft;

        // Initialize
        vector<SDFT<float, float>> engines(NUM_SIGNALS,
                                           SDFT<float, float>(numSamples, Window::Hann, 0));
        vector<vector<ComplexType>> outputs(NUM_SIGNALS, vector<ComplexType>(numSamples));

        // Start reading samples and estimating phasors and frequencies
        AdcSample sample;
        while (fread(&sample, sizeof(sample), 1, stdin) > 0) {
            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                engines[i].sdft(sample.ch[i], outputs[i].data());
            }

            auto &cur = measurements[bufferIndex];
            const auto &prv = measurements[PREV(bufferIndex)];

            cur.adcSample = sample;
            cur.freq = 0;
            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                size_t phasorIndex = 1;
                float maxMagnitude = 0;
                for (size_t j = 1, half = numSamples >> 1; j < half; ++j) {
                    float magnitude = abs(outputs[i][j]);
                    if (magnitude > maxMagnitude) {
                        maxMagnitude = magnitude;
                        phasorIndex = j;
                    }
                }
                cur.phasors[i] = outputs[i][1];
                cur.freq += std::fmod(std::arg(cur.phasors[i]) - std::arg(prv.phasors[i])
                                              + (2 * PI<float>),
                                      (2 * PI<float>));
            }
            cur.freq /= (cur.adcSample.delta / 1e6);
            cur.freq /= (2 * PI<float>);
            cur.freq /= NUM_SIGNALS;

            // printf("%s\n", ((stringstream() << cur.phasors[0]).str()).c_str());
            std::cout << cur.phasors[0] << " -- polar(" << std::abs(cur.phasors[0]) << ","
                      << std::fmod((std::arg(cur.phasors[0]) * RAD2DEG<float>) + 360, 360) << ")\n";
            std::cout << cur.phasors[1] << " -- polar(" << std::abs(cur.phasors[1]) << ","
                      << std::fmod((std::arg(cur.phasors[1]) * RAD2DEG<float>) + 360, 360) << ")\n";
            std::cout << cur.phasors[2] << " -- polar(" << std::abs(cur.phasors[2]) << ","
                      << std::fmod((std::arg(cur.phasors[2]) * RAD2DEG<float>) + 360, 360) << ")\n";
            std::cout << '\n';

            bufferIndex = NEXT(bufferIndex);
        }
    }
#undef NEXT
#undef PREV
}

int main(int argc, char *argv[])
{
    // Strategy to use, defaulting to SDFT
    StrategyChoice strategy = StrategySDFT;
    if (argc > 1) {
        if (string(argv[1]) == "fft") {
            strategy = StrategyFFT;
        }
    }

    // Float type to use, defaulting to float
    FloatTypeChoice floatType = FloatTypeFloat;
    if (argc > 2) {
        if (string(argv[2]) == "double") {
            floatType = FloatTypeDouble;
        }
    }

    // Number of samples to process
    size_t numSamples = 24;
    if (argc > 3) {
        numSamples = std::stoul(argv[3]);
    }

    if (floatType == FloatTypeFloat) {
        work<float>(strategy, numSamples);
    } else /* (floatType == FloatTypeDouble) */ {
        work<double>(strategy, numSamples);
    }

    return 0;
}