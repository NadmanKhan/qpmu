#include "worker.h"

Worker::Worker() : QThread()
{
    m_socket = new QUdpSocket(this);
}

void Worker::run()
{
    while (fread(&m_estimations, sizeof(m_estimations), 1, stdin)) {
        m_socket->writeDatagram((const char *)(void *)(&m_estimations), sizeof(m_estimations),
                                QHostAddress::LocalHost, 12345);
    }
}

void Worker::getEstimations(qpmu::Estimation &out_estimations)
{
    out_estimations = m_estimations;
}
