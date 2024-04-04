#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>
#include <QTimer>
#include <QBoxLayout>

#include "worker.h"
#include "signal_info_model.h"

class WaveformView : public QWidget
{
    Q_OBJECT
public:
    WaveformView(Worker *worker, QWidget *parent = nullptr);

private slots:
    void updateSeries();

private:
    Worker *m_worker;
    QList<QSplineSeries *> m_listSplineSeries;
    QList<QVector<QPointF>> m_listSplineSeriesPoints;

    QValueAxis *m_axisTime;
    QValueAxis *m_axisVoltage;
    QValueAxis *m_axisCurrent;

    QTimer m_timer;
};

#endif // WAVEFORM_VIEW_H
