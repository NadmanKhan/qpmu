#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "app.h"
#include "settings_models.h"
#include "phasor_server.h"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include <array>
#include <functional>

using SampleStore = std::array<qpmu::Sample, 32>;

using SampleReadBuffer = std::array<qpmu::Sample, 1>;

class DataProcessor : public QThread
{
    Q_OBJECT

public:
    DataProcessor();

    void run() override;

    const qpmu::Estimation &currentEstimation()
    {
        QMutexLocker locker(&m_mutex);
        return m_estimator->currentEstimation();
    }

    const SampleStore &currentSampleStore()
    {
        QMutexLocker locker(&m_mutex);
        return m_sampleStore;
    }

    void replacePhasorServer();
    PhasorServer *phasorServer() const { return m_server; }

public slots:
    void getCurrent(qpmu::Sample &sample, qpmu::Estimation &estimation)
    {
        QMutexLocker locker(&m_mutex);
        sample = m_estimator->currentSample();
        estimation = m_estimator->currentEstimation();
    }

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;
    SampleStore m_sampleStore = {};
    SampleReadBuffer m_sampleReadBuffer = {};
    std::function<uint64_t(QString &)> m_readSamples = nullptr;
    struct
    {
        qpmu::ADCStreamBuffer streamBuf = {};
        uint64_t counter = {};
        int64_t lastTimeUsec = {};
        int64_t lastBufTimeNsec = {};
    } m_rawReadState;

    PhasorServer *m_server = nullptr;
    QThread *m_serverThread = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
