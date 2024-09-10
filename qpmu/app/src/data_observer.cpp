#include "data_observer.h"
#include "src/settings_models.h"

DataObserver::DataObserver(QObject *parent) : QObject(parent)
{

    connect(APP->timer(), &QTimer::timeout, this, &DataObserver::update);
}

void DataObserver::update()
{
    VisualisationSettings settings;
    settings.load();

    ++m_sampleUpdateCounter;
    if (m_sampleUpdateCounter * App::UpdateIntervalFactorMs >= settings.sampleUpdateIntervalMs) {
        m_sampleUpdateCounter = 0;
        m_sample = APP->dataProcessor()->lastSample();
        emit sampleUpdated(m_sample);
    }

    ++m_estimationUpdateCounter;
    if (m_estimationUpdateCounter * App::UpdateIntervalFactorMs
        >= settings.estimationUpdateIntervalMs) {
        m_estimationUpdateCounter = 0;
        m_estimation = APP->dataProcessor()->lastEstimation();
        emit estimationUpdated(m_estimation);
    }
}