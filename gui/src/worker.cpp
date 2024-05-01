#include "worker.h"

#include <iostream>
#include <iomanip>
#include <cstring>

using std::cout, std::cerr;

inline std::complex<double> *to_complex(fftw_complex *v)
{
    return reinterpret_cast<std::complex<double> *>(v);
}

Worker::Worker(QIODevice *adc) : QThread(), m_adc(adc)
{
    assert(m_adc != nullptr);

    m_adc->moveToThread(this);

    for (int i = 0; i < NUM_SIGNALS; ++i) {
        input[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        output[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        for (int j = 0; j < N; ++j) {
            // zero out
            input[i][j][0] = 0;
            input[i][j][1] = 0;
        }
        plans[i] = fftw_plan_dft_1d(N, input[i], output[i], FFTW_FORWARD, FFTW_ESTIMATE);
    }

    connect(m_adc, &QIODevice::readyRead, this, &Worker::readAndParse);
}

Worker::~Worker()
{
    for (int i = 0; i < NUM_SIGNALS; ++i) {
        fftw_free(input[i]);
        fftw_free(output[i]);
        fftw_destroy_plan(plans[i]);
    }
}

void Worker::run()
{
    if (auto asProcess = qobject_cast<QProcess *>(m_adc)) {
        asProcess->start();
    }
    while (m_adc->waitForReadyRead(-1)) {
        readAndParse();
    }
    return;
    while (fread(&m_measurement, sizeof(m_measurement), 1, stdin)) { }
}

void Worker::readAndParse()
{
    for (char ch : m_adc->readAll()) {
        if (ch == '=') {
            switch (prevByte) {
            case 'o':
            case 'm':
            case 'x':
                // "*no" or "*num" or "*idx" or "*index"
                tokenIndex = 0;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
                // "ch{n}"
                tokenIndex = 1 + (prevByte - '0');
                break;
            case 's':
                // "ts"
                tokenIndex = 7;
                break;
            case 'a':
                // "delta"
                tokenIndex = 8;
                break;
            }
            curSample[tokenIndex] = 0;

        } else if (isdigit(ch) && prevByte != 'h') {
            curSample[tokenIndex] *= 10;
            curSample[tokenIndex] += (ch - '0');

        } else if (ch == '\n') {
            addCurSample();
        }

        prevByte = ch;
    }
}

void Worker::addCurSample()
{
    // 1. fill input
    {
        QMutexLocker locker(&mutex);
        sampleBuffer[sampleIndex] = curSample;
        for (int i = 0; i < NUM_SIGNALS; ++i) {
            int index = nextIndex(sampleIndex);
            for (int j = 0; j < N; ++j) {
                input[i][j][0] = sampleBuffer[index][1 + i];
                index = nextIndex(index);
            }
        }
    }

    // 2. execute plans
    for (int i = 0; i < NUM_SIGNALS; ++i) {
        fftw_execute(plans[i]);
    }

    // 3. extract the phasor from output and write it to buffer

    {
        QMutexLocker locker(&mutex);
        for (int i = 0; i < NUM_SIGNALS; ++i) {
                       double maxNorm = 0;
                       int maxIdx = 1;
                       for (int j = 1; j < N / 2; ++j) {
                           auto norm = std::norm(*to_complex(&output[i][j]));
                           if (maxNorm < norm) {
                               maxNorm = norm;
                               maxIdx = j;
                           }
                       }
            phasorBuffer[i][sampleIndex] = *to_complex(&output[i][maxIdx]);
        }
    }

    sampleIndex = nextIndex(sampleIndex);
}

void Worker::getEstimations(std::array<std::complex<double>, NUM_SIGNALS> &out_phasors,
                            double &out_omega)
{
    QMutexLocker locker(&mutex);
    int readIdx = prevIndex(sampleIndex);
    int prevReadIdx = prevIndex(readIdx);

    for (int i = 0; i < NUM_SIGNALS; ++i) {
        out_phasors[i] = phasorBuffer[i][readIdx];
    }

    double sum_omega_micros = 0;

    for (int i = 0; i < N; ++i) {
        if (i == sampleIndex || i == nextIndex(sampleIndex))
            continue;
        const auto &theta2 = std::arg(phasorBuffer[0][i]);
        const auto &theta1 = std::arg(phasorBuffer[0][prevIndex(i)]);
        auto phaseDiff = std::fmod((theta1 - theta2) + (2 * M_PI), (2 * M_PI));
        sum_omega_micros += (phaseDiff / sampleBuffer[i][8] /* timedelta */);
    }
    out_omega = (sum_omega_micros * 1e6 / (N - 2) / N * F);
    //    qDebug() << (out_omega / (2 * M_PI));
}

Measurement Worker::measurement() const
{
    return m_measurement;
}

constexpr int Worker::nextIndex(int currIndex)
{
    return (currIndex + 1) % N;
}

constexpr int Worker::prevIndex(int currIndex)
{
    return ((currIndex - 1) % N + N) % N;
}
