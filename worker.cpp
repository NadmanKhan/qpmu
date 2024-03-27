#include "worker.h"

#include <iostream>
#include <iomanip>
using std::cout;

std::complex<double> to_complex(fftw_complex v) {
    return std::complex<double>(v[0], v[1]);
}

Worker::Worker(QIODevice *adc)
    : QThread(),
      m_adc(adc)
{
    assert(m_adc != nullptr);

    for (int i = 0; i < 6; ++i) {
        fft_in[i] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
        fft_out[i] = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
        for (int j = 0; j < N; ++j) {
            // zero out
            fft_in[i][j][0] = 0;
            fft_in[i][j][1] = 0;
        }
        fft_plans[i] = fftw_plan_dft_1d(N, fft_in[i], fft_out[i],
                                       FFTW_FORWARD, FFTW_ESTIMATE);
    }

//    connect(m_adc, &QIODevice::readyRead, this, &Worker::read);
}

Worker::~Worker()
{
    for (int i = 0; i < 6; ++i) {
        fftw_free(fft_in[i]);
        fftw_free(fft_out[i]);
        fftw_destroy_plan(fft_plans[i]);
    }
}

void Worker::run()
{
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
                // seq_no
                bufferInnerIndex = 0;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
                // ch{n}
                bufferInnerIndex = 1 + (prevByte - '0');
                break;
            case 's':
                // ts
                bufferInnerIndex = 7;
                break;
            case 'a':
                // delta
                bufferInnerIndex = 8;
                // subtract from tsDeltaSum
                QMutexLocker locker(&mutex);
                tsDeltaSum -= sampleBuffer[bufferOuterIndex][8];
                break;
            }
            sampleBuffer[bufferOuterIndex][bufferInnerIndex] = 0;
        } else if (isdigit(ch)) {
            sampleBuffer[bufferOuterIndex][bufferInnerIndex] *= 10;
            sampleBuffer[bufferOuterIndex][bufferInnerIndex] += (ch - '0');
        } else if (ch == '\n') {

            // compute phasor

            // 1. fill fft_in
            for (int i = 0; i < 6; ++i) {
                double max_value = 0;
                int index = (bufferOuterIndex + 1) % N;
                for (int j = 0; j < N; ++j) {
                    fft_in[i][j][0] = sampleBuffer[index][1 + i];
                    max_value = std::max(max_value, fft_in[i][j][0]);
                    index = (index + 1) % N;
                }
                auto half_max = max_value / 2;
                for (int j = 0; j < N; ++j) {
                    fft_in[i][j][0] -= half_max;
                }
            }


            // 2. execute fft_plans
            for (int i = 0; i < 6; ++i) {
                fftw_execute(fft_plans[i]);
            }

            QMutexLocker locker(&mutex);

            // 3. extract the phasor from fft_out and write it to buffer

            for (int i = 0; i < 6; ++i) {
//                std::complex<double> phasor = 0;
//                for (int j = 0; j < N; ++j) {
//                    phasor += to_complex(fft_out[i][j]);
//                }
//                phasorBuffer[i][bufferOuterIndex] = phasor;
                std::complex<double> phasor = 0;
                double maxMag = 0;
                for (int j = 0; j < N; ++j) {
                    auto p = to_complex(fft_out[i][j]);
                    if (maxMag < std::abs(p)) {
                        phasor = p;
                    }
                }
                phasorBuffer[i][bufferOuterIndex] = phasor;
            }

            // 4. add to tsDeltaSum for frequency computation.
            // the previous value at `sampleBuffer[bufferOuterIndex][8]`
            // has already been subtracted.
            tsDeltaSum = sampleBuffer[bufferOuterIndex][8];

            for (int i = 0; i < 9; ++i) {
                if (i == 0) {
                    cout << "seq_no";
                } else if (i <= 6) {
                    cout << "ch" << i - 1;
                } else if (i == 7) {
                    cout << "ts";
                } else {
                    cout << "delta";
                }
                cout << "=";
                if (1 <= i && i <= 6) {
                    cout << std::setw(4) << std::right;
                }
                cout << sampleBuffer[bufferOuterIndex][i] << ",";
                if (!(1 <= i && i <= 6)) {
                    cout << "\t";
                }
            }
            cout << "\n";

            for (int i = 0; i < 6; ++i) {
                auto phasor = phasorBuffer[i][bufferOuterIndex];
                QPointF point = {(std::arg(phasor) * factorRadToDeg) + 180,
                                 std::abs(phasor) / N};
                cout << std::fixed << std::setprecision(5);
                cout << i << ":("
                     << std::setw(10) << std::right << point.x() << ","
                     << std::setw(10) << std::right << point.y() << ") ";
            }
            cout << "\n";

            bufferOuterIndex = (bufferOuterIndex + 1) % N;
        }

        prevByte = ch;
    }
}


QVariantList Worker::seriesInfoList(const QString& dataType)
{
    QVariantList res;
    for (int i = 0; i < 3; ++i) {
        QVariantMap item;
        item[QStringLiteral("name")] = QStringLiteral("V") + char('A' + i);
        item[QStringLiteral("color")] = std::array<QString, 3>{"#ff0000", "#00ff00", "#0000ff"}[i];
        item[QStringLiteral("visible")] = true;
        item[QStringLiteral("type")] = QStringLiteral("voltage");
        res << item;
    }
    for (int i = 0; i < 3; ++i) {
        QVariantMap item;
        item[QStringLiteral("name")] = QStringLiteral("I") + char('A' + i);
        item[QStringLiteral("color")] = std::array<QString, 3>{"#ffff00", "#00ffff", "#ff00ff"}[i];
        item[QStringLiteral("visible")] = true;
        item[QStringLiteral("type")] = QStringLiteral("current");
        res << item;
    }
    return res;
}

void Worker::updatePoints(const QList<QLineSeries *> &series, const QString& dataType)
{
    if (dataType == QStringLiteral("phasor")) {
        QMutexLocker locker(&mutex);
        int at = (bufferOuterIndex - 1 + N) % N;
        for (int i = 0; i < series.size(); ++i) {
            auto phasor = phasorBuffer[i][at];
            QPointF point = {(std::arg(phasor) * factorRadToDeg) + 180,
                             std::abs(phasor) / N};
            series[i]->replace(QVector<QPointF>{{0, 0}, point});
        }

    } else if (dataType == QStringLiteral("waveform")) {
        QMutexLocker locker(&mutex);
        auto frequency = tsDeltaSum / (double)N;
        qDebug() << "freq: " << frequency;
        int at = (bufferOuterIndex - 1 + N) % N;
        for (int i = 0; i < series.size(); ++i) {
            auto phasor = phasorBuffer[i][at];
            auto phaseAngle = std::arg(phasor);
            auto amplitude = std::abs(phasor);
            // we have (frequency, amplitude, phaseAngle)
            // construct the sinusoid (cosine function)
            // and show it over 0.1 second interval at 0.005 second quanta
            const int n = 50;
            const double delta = 0.1 / n;
            QVector<QPointF> points(n + 1);
            for (int ti = 0; ti < points.size(); ++ti) {
                double t = ti * delta;
                auto xt = amplitude * cos(2 * pi * frequency * t + phaseAngle);
                points[ti] = QPointF(t, xt);
            }
            series[i]->replace(points);
        }
    }
}

constexpr double Worker::phasorMag(fftw_complex phasor)
{
    return sqrt(phasor[0] * phasor[0] + phasor[1] * phasor[1]) / N;
}

constexpr double Worker::phasorAng(fftw_complex phasor)
{
    return atan2(phasor[1], phasor[0]) * (180 / pi);
}
