#include "worker.h"

Worker::Worker() : QThread() { }

void Worker::run()
{
    while (fread(&m_measurement, sizeof(m_measurement), 1, stdin)) { }
}

void Worker::getMeasurement(qpmu::Measurement &out_measurement)
{
    out_measurement = m_measurement;
}
