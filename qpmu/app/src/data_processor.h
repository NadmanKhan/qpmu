#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "app.h"

#include <QThread>
#include <QIODevice>
#include <QMutex>

#include <functional>
#include <array>

class DataProcessor : public QThread
{
    Q_OBJECT

public:
    explicit DataProcessor();

    void run() override;
    const qpmu::Synchrophasor &lastSynchrophasor();
    const qpmu::Sample &lastSample();
    const std::array<qpmu::Float, qpmu::SignalCount> &channelMagnitudes();

public slots:
    void updateSampleSource();

signals:
    void newSampleObtained(qpmu::Sample sample);
    void newSynchrophasorObtained(qpmu::Synchrophasor synchrophasor);

private:
    QMutex m_mutex;

    qpmu::Sample m_sample;
    qpmu::Synchrophasor m_synchrophasor;

    bool m_sampleSourceIsBinary = false;
    QIODevice *m_sampleSourceDevice = nullptr;
    std::function<bool()> m_sampleSourceDeviceReady = nullptr;

    qpmu::Estimator *m_estimator = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
