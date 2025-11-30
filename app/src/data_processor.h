#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "app.h"
#include "phasor_server.h"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>

#include <array>

using SampleWindow = std::array<qpmu::Sample, 128>;
using EstimationWindow = std::array<qpmu::Estimation, 128>;

using SampleReadBuffer = std::array<qpmu::Sample, 1>;

class DataProcessor : public QThread
{
    Q_OBJECT

public:
    DataProcessor();

    void run() override;

    uint64_t readSamples(QString &error) const;

    const qpmu::Estimation &lastEstimation();
    const qpmu::Estimation lastEstimationFiltered();
    const qpmu::Sample &lastSample();
    const SampleWindow &sampleWindow();

    void replacePhasorServer();
    PhasorServer *phasorServer() const { return m_server; }

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;
    EstimationWindow m_estimations = {};
    SampleWindow m_samples = {};
    SampleReadBuffer m_sampleReadBuffer = {};
    bool m_readBinary = false;
    FILE *inputFile = stdin;

    PhasorServer *m_server = nullptr;
    QThread *m_serverThread = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
