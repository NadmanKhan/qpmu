#include "data_observer.h"
#include "src/data_processor.h"

DataObserver::DataObserver()
{
    connect(APP->timer(), &QTimer::timeout, this, &DataObserver::update);
}

void DataObserver::update()
{

    ++m_updateCounter;

    if (m_updateCounter * App::TimerIntervalMs >= App::DataViewUpdateIntervalMs) {
        m_updateCounter = 0;
        emit sampleBufferUpdated(APP->dataProcessor()->currentSampleBuffer());
        emit estimationUpdated(APP->dataProcessor()->currentEstimation());
    }
}