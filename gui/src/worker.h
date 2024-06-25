#ifndef WORKER_H
#define WORKER_H

#include <QThread>
#include <QtNetwork/QUdpSocket>

#include "qpmu/common.h"

class Worker : public QThread
{
    Q_OBJECT

public:
    explicit Worker();
    void run() override;

public slots:
    Q_INVOKABLE void getEstimations(qpmu::Estimation &out_measurement);

private:
    qpmu::Estimation m_estimations;
    QUdpSocket *m_socket;
};

#endif // WORKER_H
