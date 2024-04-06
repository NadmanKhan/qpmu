#include "worker.h"

#include <iostream>
#include <iomanip>
using std::cout;

inline std::complex<double> *to_complex(fftw_complex *v)
{
    return reinterpret_cast<std::complex<double> *>(v);
}

Worker::Worker(QIODevice *adc) : QThread(), m_adc(adc)
{
    assert(m_adc != nullptr);

    m_adc->moveToThread(this);

    for (int i = 0; i < nsignals; ++i) {
        fft_in[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        fft_out[i] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
        for (int j = 0; j < N; ++j) {
            // zero out
            fft_in[i][j][0] = 0;
            fft_in[i][j][1] = 0;
        }
        fft_plans[i] = fftw_plan_dft_1d(N, fft_in[i], fft_out[i], FFTW_FORWARD, FFTW_ESTIMATE);
    }

    //    connect(m_adc, &QIODevice::readyRead, this, &Worker::read);
}

Worker::~Worker()
{
    for (int i = 0; i < nsignals; ++i) {
        fftw_free(fft_in[i]);
        fftw_free(fft_out[i]);
        fftw_destroy_plan(fft_plans[i]);
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
                bufCol = 0;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
                // "ch{n}"
                bufCol = 1 + (prevByte - '0');
                break;
            case 's':
                // "ts"
                bufCol = 7;
                break;
            case 'a':
                // "delta"
                bufCol = 8;
                break;
            }
            sampleBuffer[bufRow][bufCol] = 0;
        } else if (isdigit(ch) && prevByte != 'h') {
            sampleBuffer[bufRow][bufCol] *= 10;
            sampleBuffer[bufRow][bufCol] += (ch - '0');
        } else if (ch == '\n') {

            // compute phasor

            // 1. fill fft_in
            for (int i = 0; i < nsignals; ++i) {
                double max_value = 0;
                int bufIdx = (bufRow + 1) % N;
                for (int j = 0; j < N; ++j) {
                    fft_in[i][j][0] = sampleBuffer[bufIdx][1 + i];
                    max_value = std::max(max_value, fft_in[i][j][0]);
                    bufIdx = (bufIdx + 1) % N;
                }
                auto half_max = max_value / 2;
                for (int j = 0; j < N; ++j) {
                    fft_in[i][j][0] -= half_max;
                }
            }

            // 2. execute fft_plans
            for (int i = 0; i < nsignals; ++i) {
                fftw_execute(fft_plans[i]);
            }

            QMutexLocker locker(&mutex);

            // 3. extract the phasor from fft_out and write it to buffer

            for (int i = 0; i < nsignals; ++i) {
                //                std::complex<double> phasor = 0;
                //                for (int j = 0; j < N; ++j) {
                //                    phasor += *to_complex(&fft_out[i][j]);
                //                }
                //                phasorBuffer[i][bufferOuterIndex] = phasor;
                std::complex<double> phasor = 0;
                double maxMag = 0;
                for (int j = 0; j < N; ++j) {
                    if (maxMag < std::abs(*to_complex(&fft_out[i][j]))) {
                        phasor = *to_complex(&fft_out[i][j]);
                    }
                }
                phasorBuffer[i][bufRow] = phasor;
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

            bufRow = (bufRow + 1) % N;
        }

        prevByte = ch;
    }
}

void Worker::getEstimations(std::array<std::complex<double>, nsignals> &out_phasors,
                            std::array<double, nsignals> &out_frequencies)
{
    QMutexLocker locker(&mutex);
    int readIdx = (bufRow - 1 + N) % N;
    int prevReadIdx = (readIdx - 1 + N) % N;
    for (int i = 0; i < nsignals; ++i) {
        for (int j = prevIndex(bufRow), k = prevIndex(j); k != bufRow;
             j = prevIndex(j), k = prevIndex(k)) { }
        out_frequencies[i] = (std::abs(std::arg(phasorBuffer[i][readIdx])
                                       - std::arg(phasorBuffer[i][prevReadIdx]))
                              / sampleBuffer[readIdx][8])
                * (1e6 / (2 * pi));

        out_phasors[i] = phasorBuffer[i][readIdx];
    }
}

constexpr int Worker::nextIndex(int currIndex)
{
    return (currIndex + 1) % N;
}

constexpr int Worker::prevIndex(int currIndex)
{
    return ((currIndex - 1) % N + N) % N;
}
