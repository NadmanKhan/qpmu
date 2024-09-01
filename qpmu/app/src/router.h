#ifndef ROUTER_H
#define ROUTER_H

#include <QThread>
#include <QIODevice>
#include <QMutex>

#include <functional>
#include <array>

#include "qpmu/common.h"
#include "qpmu/estimator.h"
#include "app.h"

class Router : public QThread
{
    Q_OBJECT

public:
    explicit Router();

    void run() override;
    qpmu::Synchrophasor lastSynchrophasor();

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

#endif // ROUTER_H
