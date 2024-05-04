#ifndef WORKER_H
#define WORKER_H

#include <QThread>

#include "qpmu/common.h"

class Worker : public QThread
{
    Q_OBJECT

public:
    explicit Worker();
    void run() override;

public slots:
    Q_INVOKABLE void getEstimations(qpmu::Estimations &out_measurement);

private:
    qpmu::Estimations m_estimations;
};

#endif // WORKER_H
