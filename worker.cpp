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

    for (int i = 0; i < nsignals; ++i) {
        input[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        output[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        for (int j = 0; j < N; ++j) {
            // zero out
            input[i][j][0] = 0;
            input[i][j][1] = 0;
        }
        plans[i] = fftw_plan_dft_1d(N, input[i], output[i], FFTW_FORWARD, FFTW_ESTIMATE);
    }

    //    connect(m_adc, &QIODevice::readyRead, this, &Worker::read);
}

Worker::~Worker()
{
    for (int i = 0; i < nsignals; ++i) {
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
        read();
    }
}

void Worker::read()
{
    for (char ch : m_adc->readAll()) {
        if (ch == '=') {
            switch (prevByte) {
            case 'o':
                // "seq_no"
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

            // compute phasor

            // 1. fill fft_in
            {
                QMutexLocker locker(&mutex);
                sampleBuffer[sampleIndex] = curSample;
                for (int i = 0; i < nsignals; ++i) {
                    int index = nextIndex(sampleIndex);
                    for (int j = 0; j < N; ++j) {
                        input[i][j][0] = sampleBuffer[index][1 + i];
                        index = nextIndex(index);
                    }
                }
            }

            // 2. execute fft_plans
            for (int i = 0; i < nsignals; ++i) {
                fftw_execute(plans[i]);
            }

            // 3. extract the phasor from fft_out and write it to buffer

            {
                QMutexLocker locker(&mutex);
                for (int i = 0; i < nsignals; ++i) {
                    std::complex<double> phasor = 0;
                    double maxMag = 0;
                    for (int j = 0; j < N; ++j) {
                        if (maxMag < std::abs(*to_complex(&output[i][j]))) {
                            phasor = *to_complex(&output[i][j]);
                        }
                    }
                    phasorBuffer[i][sampleIndex] = phasor;
                }
            }

            //            for (int i = 0; i < 9; ++i) {
            //                if (i == 0) {
            //                    cout << "seq_no";
            //                } else if (i <= 6) {
            //                    cout << "ch" << i - 1;
            //                } else if (i == 7) {
            //                    cout << "ts";
            //                } else {
            //                    cout << "delta";
            //                }
            //                cout << "=";
            //                if (1 <= i && i <= 6) {
            //                    cout << std::setw(4) << std::right;
            //                }
            //                cout << sampleBuffer[bufferOuterIndex][i] << ",";
            //                if (!(1 <= i && i <= 6)) {
            //                    cout << "\t";
            //                }
            //            }
            //            cout << "\n";

            //            for (int i = 0; i < 6; ++i) {
            //                auto phasor = phasorBuffer[i][bufRow];
            //                QPointF point = { (std::arg(phasor) * factorRadToDeg),
            //                std::abs(phasor) / N }; cout << std::fixed << std::setprecision(1);
            //                cout << i << ":(" << std::setw(7) << std::right << point.x() << "," <<
            //                std::setw(7)
            //                     << std::right << point.y() << ") ";
            //            }
            //            cout << "\n";

            sampleIndex = nextIndex(sampleIndex);
        }

        prevByte = ch;
    }
}

void Worker::getEstimations(std::array<std::complex<double>, nsignals> &out_phasors, double &out_ω)
{
    QMutexLocker locker(&mutex);
    int readIdx = prevIndex(sampleIndex);
    int prevReadIdx = prevIndex(readIdx);

    for (int i = 0; i < nsignals; ++i) {
        out_phasors[i] = phasorBuffer[i][readIdx];
    }

    double sum_ω_micros = 0;

    for (int i = 0; i < N; ++i) {
        if (i == sampleIndex || i == nextIndex(sampleIndex))
            continue;
        const auto &θ2 = std::arg(phasorBuffer[0][i]);
        const auto &θ1 = std::arg(phasorBuffer[0][prevIndex(i)]);
        auto phaseDiff = std::fmod((θ1 - θ2) + (2 * M_PI), (2 * M_PI));
        sum_ω_micros += (phaseDiff / sampleBuffer[i][8] /* timedelta */);
    }
    out_ω = (sum_ω_micros * 1e6 / (N - 2));
}

constexpr int Worker::nextIndex(int currIndex)
{
    return (currIndex + 1) % N;
}

constexpr int Worker::prevIndex(int currIndex)
{
    return ((currIndex - 1) % N + N) % N;
}
