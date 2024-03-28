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

    static constexpr double phasorMag(fftw_complex phasor);
    static constexpr double phasorAng(fftw_complex phasor);

public slots:
    void read();

    Q_INVOKABLE QVariantList seriesInfoList(const QString &dataType);
    Q_INVOKABLE void updatePoints(const QList<QLineSeries *> &series, const QString &dataType);

private:
    QMutex mutex;
    QIODevice *m_adc;

    std::array<fftw_complex *, 6> fft_in, fft_out;
    std::array<fftw_plan, 6> fft_plans;

    std::array<ADCSample, N> sampleBuffer = {};
    std::array<std::array<std::complex<double>, N>, 6> phasorBuffer = {};
    char prevByte = 0;
    int bufCol = 0;
    int bufRow = 0;
};

#endif // WORKER_H
