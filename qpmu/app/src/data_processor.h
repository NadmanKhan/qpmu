#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "settings_models.h"
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
    const qpmu::Estimation &lastEstimation();
    const qpmu::Sample &lastSample();

signals:
    void sampleSourceConnectionChanged(bool isConnected);

public slots:
    void updateSampleSource();

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;

    SampleSourceSettings m_sampleSourcesettings;

    struct
    {
        bool isConnected = false;
        bool isDataBinary = true;
        QIODevice *device = nullptr;
        QIODevice *newDevice = nullptr;
        std::function<void()> startNewDevice = nullptr;
        std::function<bool()> isDeviceReadyToRead = []() { return false; };
    } sampler;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
