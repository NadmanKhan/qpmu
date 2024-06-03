#include "worker.h"

Worker::Worker() : QThread() { }

void Worker::run()
{
    while (fread(&m_estimations, sizeof(m_estimations), 1, stdin)) { }
}

void Worker::getEstimations(qpmu::Estimation &out_estimations)
{
    out_estimations = m_estimations;
}
