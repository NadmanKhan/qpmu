#ifndef WORKER_H
#define WORKER_H

#include <QtCharts>
#include <QThread>
#include <QIODevice>
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QVariantList>
#include <QMutex>

#include <fftw3.h>

#include <complex>
#include <cmath>

#include "signal_info_model.h"

constexpr double pi = 3.14159265358979323846264338;
constexpr double factorRadToDeg = 180 / pi;
using ADCSample = std::array<uint64_t, 9>;

class Worker : public QThread
{
    Q_OBJECT

public:
    explicit Worker(QIODevice *adc);
    ~Worker();

    void run() override;

    static constexpr int N = 24;

public slots:
    void readAndParse();

    Q_INVOKABLE void getEstimations(std::array<std::complex<double>, nsignals> &out_phasors,
                                    double &out_ω);

private slots:
    void addCurSample();

private:
    static constexpr int nextIndex(int currIndex);
    static constexpr int prevIndex(int currIndex);

    QMutex mutex;
    QIODevice *m_adc;

    std::array<fftw_complex *, nsignals> input, output;
    std::array<fftw_plan, nsignals> plans;

    ADCSample curSample = {};
    std::array<ADCSample, N> sampleBuffer = {};
    std::array<std::array<std::complex<double>, N>, nsignals> phasorBuffer = {};
    char prevByte = 0;
    int tokenIndex = 0;
    int sampleIndex = 0;
};

#endif // WORKER_H
