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
    WaveformView(QTimer *updateTimer, Worker *worker, QWidget *parent = nullptr);

private slots:
    void update();

private:
    Worker *m_worker;

    QList<QSplineSeries *> m_listSplineSeries;
    QList<QVector<QPointF>> m_listSplineSeriesPoints;

    QValueAxis *m_axisTime;
    QValueAxis *m_axisVoltage;
    QValueAxis *m_axisCurrent;

    int m_timeoutCounter;
    int m_timeoutTarget;
};

#endif // WAVEFORM_VIEW_H
