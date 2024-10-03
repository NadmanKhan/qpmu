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

    ++m_updateCounter;
    if (m_updateCounter * App::TimerIntervalMs >= settings.dataViewUpdateIntervalMs) {
        m_updateCounter = 0;
        m_sample = APP->dataProcessor()->lastSample();
        m_estimation = APP->dataProcessor()->lastEstimation();
        emit sampleUpdated(m_sample);
        emit estimationUpdated(m_estimation);
    }
}