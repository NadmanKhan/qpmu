#ifndef PHASORVIEW_H
#define PHASORVIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>
#include <QList>
#include <QString>
#include <QTimer>

#include "worker.h"

class PhasorView : public QChartView
{
    Q_OBJECT
public:
    PhasorView(Worker *worker);

private slots:
    void updateSeries();

private:
    QList<QLineSeries *> m_series;
    Worker *m_worker;
};

#endif // PHASORVIEW_H
