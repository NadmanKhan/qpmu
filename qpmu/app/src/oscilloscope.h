#ifndef QPMU_APP_OSCILLOSCOPE_H
#define QPMU_APP_OSCILLOSCOPE_H

#include "qpmu/defs.h"
#include "main_page_interface.h"
#include "data_observer.h"

#include <QWidget>
#include <QtCharts>

class Oscilloscope : public QWidget, public MainPageInterface
{
    Q_OBJECT
    Q_INTERFACES(MainPageInterface)

public:
    explicit Oscilloscope(QWidget *parent = nullptr);

private slots:
    void updateView(const SampleStore &samples);

private:
    QLineSeries *m_series[qpmu::CountSignals] = {};
    QVector<QPointF> m_points[qpmu::CountSignals] = {};
    QDateTimeAxis *m_timeAxis = nullptr;
    QValueAxis *m_valueAxis = nullptr;
};

#endif // QPMU_APP_OSCILLOSCOPE_H