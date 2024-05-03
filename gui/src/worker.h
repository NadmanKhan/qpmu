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
    Q_INVOKABLE void getMeasurement(qpmu::Measurement &out_measurement);

private:
    qpmu::Measurement m_measurement;
};

#endif // WORKER_H
