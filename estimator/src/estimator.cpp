
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <limits>
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
constexpr auto PI = M_PI;

template <typename FloatType>
constexpr auto DEG2RAD = PI<FloatType> / 180;

template <typename FloatType>
constexpr auto RAD2DEG = 180 / PI<FloatType>;

template <typename FloatType = void>
struct FFTW
{
};

#define SPECIALIZE(type, suffix)                                        \
  template <>                                                           \
  constexpr auto PI<type> = M_PI##suffix;                               \
  template <>                                                           \
  constexpr auto DEG2RAD<type> = M_PI##suffix / 180;                    \
  template <>                                                           \
  constexpr auto RAD2DEG<type> = 180 / M_PI##suffix;                    \
                                                                        \
  template <>                                                           \
  struct FFTW<type>                                                     \
  {                                                                     \
    using ComplexType = fftw##suffix##_complex;                         \
    using PlanType = fftw##suffix##_plan;                               \
    static constexpr auto alloc_complex = fftw##suffix##_alloc_complex; \
    static constexpr auto malloc = fftw##suffix##_malloc;               \
    static constexpr auto plan_dft_1d = fftw##suffix##_plan_dft_1d;     \
    static constexpr auto execute = fftw##suffix##_execute;             \
    static constexpr auto destroy_plan = fftw##suffix##_destroy_plan;   \
    static constexpr auto free = fftw##suffix##_free;                   \
  };

SPECIALIZE(float, f)
SPECIALIZE(double, )
SPECIALIZE(long double, l)

void fillFrequency(Measurement &cur, const Measurement &prv, const AdcSample &adcSample)
{
    cur.freq = 0;
    for (size_t i = 0; i < NUM_SIGNALS; ++i) {
        cur.freq += std::fmod(std::arg(cur.phasors[i]) - std::arg(prv.phasors[i]) + (2 * PI<float>),
                              (2 * PI<float>));
    }
    cur.freq /= (adcSample.delta / 1e6);
    cur.freq /= (2 * PI<float>);
    cur.freq /= NUM_SIGNALS;
}

template <typename FloatType>
void work(StrategyChoice strategy)
{
    using ComplexType = complex<FloatType>;
    vector<Measurement> measurements(BUFFER_SIZE, Measurement{});

#define NEXT(index) ((index != (BUFFER_SIZE - 1)) * (index + 1))
#define PREV(index) ((index != 0) * (index - 1) + (index == 0) * (BUFFER_SIZE - 1))
    size_t bufferIndex = 0;

    std::cout << Measurement::csv_header() << '\n';

    if (strategy == StrategyFFT) {

        vector<typename FFTW<FloatType>::ComplexType *> inputs(NUM_SIGNALS);
        vector<typename FFTW<FloatType>::ComplexType *> outputs(NUM_SIGNALS);
        vector<typename FFTW<FloatType>::PlanType> plans(NUM_SIGNALS);
        for (size_t i = 0; i < NUM_SIGNALS; ++i) {
            inputs[i] = FFTW<FloatType>::alloc_complex(BUFFER_SIZE);
            outputs[i] = FFTW<FloatType>::alloc_complex(BUFFER_SIZE);
            plans[i] = FFTW<FloatType>::plan_dft_1d(BUFFER_SIZE, inputs[i], outputs[i],
                                                    FFTW_FORWARD, FFTW_ESTIMATE);
            for (size_t j = 0; j < BUFFER_SIZE; ++j) {
                inputs[i][j][0] = 0;
                inputs[i][j][1] = 0;
            }
        }

        AdcSample sample;
        while (fread(&sample, sizeof(sample), 1, stdin) > 0) {
            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                for (int j = 1; j < BUFFER_SIZE; ++j) {
                    inputs[i][j - 1][0] = inputs[i][j][0];
                    inputs[i][j - 1][1] = inputs[i][j][1];
                }
                inputs[i][BUFFER_SIZE - 1][0] = sample.ch[i];
            }
            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                FFTW<FloatType>::execute(plans[i]);
            }

            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                size_t phasorIndex = 1;
                FloatType maxNorm = 0;
                for (size_t j = 1, half = BUFFER_SIZE >> 1; j < half; ++j) {
                    FloatType norm = (outputs[i][j][0] * outputs[i][j][0]
                                      + outputs[i][j][1] * outputs[i][j][1]);
                    if (norm > maxNorm) {
                        maxNorm = norm;
                        phasorIndex = j;
                    }
                }
                measurements[bufferIndex].phasors[i] =
                        ComplexType(outputs[i][phasorIndex][0], outputs[i][phasorIndex][1]);
                measurements[bufferIndex].phasors_polar[i] = {
                    std::abs(measurements[bufferIndex].phasors[i]) / BUFFER_SIZE,
                    std::fmod((std::arg(measurements[bufferIndex].phasors[i])
                               * RAD2DEG<FloatType>)+360,
                              360)
                };
            }
            fillFrequency(measurements[bufferIndex], measurements[PREV(bufferIndex)], sample);

            const auto &cur = measurements[bufferIndex];
            // printf("%s\n", ((stringstream() << cur.phasors[0]).str()).c_str());
            // std::cout << cur.phasors[0] << " -- polar(" << std::abs(cur.phasors[0]) << ","
            //           << std::fmod((std::arg(cur.phasors[0]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << cur.phasors[1] << " -- polar(" << std::abs(cur.phasors[1]) << ","
            //           << std::fmod((std::arg(cur.phasors[1]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << cur.phasors[2] << " -- polar(" << std::abs(cur.phasors[2]) << ","
            //           << std::fmod((std::arg(cur.phasors[2]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << '\n';

            // fwrite(&measurements[bufferIndex], sizeof(measurements[bufferIndex]), 1, stdout);
            std::cout << to_csv(measurements[bufferIndex]) << '\n';

            bufferIndex = NEXT(bufferIndex);
        }

    } else /* (strategy == StrategySDFT) */ {
        using namespace sdft;

        // Initialize
        vector<SDFT<FloatType, FloatType>> engines(
                NUM_SIGNALS, SDFT<FloatType, FloatType>(BUFFER_SIZE, Window::Hann, 0));
        vector<vector<ComplexType>> outputs(NUM_SIGNALS, vector<ComplexType>(BUFFER_SIZE));

        // Start reading samples and estimating phasors and frequencies
        AdcSample sample;
        while (fread(&sample, sizeof(sample), 1, stdin) > 0) {
            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                engines[i].sdft(sample.ch[i], outputs[i].data());
            }

            for (size_t i = 0; i < NUM_SIGNALS; ++i) {
                size_t phasorIndex = 1;
                FloatType maxNorm = 0;
                for (size_t j = 1, half = BUFFER_SIZE >> 1; j < half; ++j) {
                    FloatType norm = std::norm(outputs[i][j]);
                    if (norm > maxNorm) {
                        maxNorm = norm;
                        phasorIndex = j;
                    }
                }
                measurements[bufferIndex].phasors[i] = outputs[i][1];
            }
            fillFrequency(measurements[bufferIndex], measurements[PREV(bufferIndex)], sample);

            const auto &cur = measurements[bufferIndex];
            // printf("%s\n", ((stringstream() << cur.phasors[0]).str()).c_str());
            // std::cout << cur.phasors[0] << " -- polar(" << std::abs(cur.phasors[0]) << ","
            //           << std::fmod((std::arg(cur.phasors[0]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << cur.phasors[1] << " -- polar(" << std::abs(cur.phasors[1]) << ","
            //           << std::fmod((std::arg(cur.phasors[1]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << cur.phasors[2] << " -- polar(" << std::abs(cur.phasors[2]) << ","
            //           << std::fmod((std::arg(cur.phasors[2]) * RAD2DEG<FloatType>)+360, 360)
            //           << ")\n";
            // std::cout << '\n';

            // fwrite(&measurements[bufferIndex], sizeof(measurements[bufferIndex]), 1, stdout);
            std::cout << to_csv(measurements[bufferIndex]) << '\n';

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

    if (floatType == FloatTypeFloat) {
        work<float>(strategy);
    } else /* (floatType == FloatTypeDouble) */ {
        work<double>(strategy);
    }

    return 0;
}