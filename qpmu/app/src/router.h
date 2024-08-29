#ifndef ROUTER_H
#define ROUTER_H

#include <QThread>
#include <QIODevice>
#include <QMutex>

#include <functional>

#include "qpmu/common.h"
#include "qpmu/estimator.h"
#include "app.h"

class Router : public QThread
{
    Q_OBJECT

public:
    explicit Router();

    void run() override;
    qpmu::Synchrophasor currentSynchrophasor();

public slots:
    void updateSampleSource();

private:
    QMutex m_mutex;

    qpmu::Synchrophasor m_synchrophasor;

    qpmu::SampleSourceConfig m_sampleSourceConfig = qpmu::conf::SampleSourceConfig;
    QIODevice *m_sampleSourceDevice = nullptr;
    std::function<bool()> m_sampleSourceDeviceReady = nullptr;

    qpmu::Estimator *m_estimator = nullptr;
};

#endif // ROUTER_H
