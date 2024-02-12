#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <array>

#include <QtCharts>
#include <QTimer>

#include "App.h"
#include "SignalInfo.h"
#include "WithDock.h"

class WaveformView: public QChartView, public WithDock
{
Q_OBJECT Q_INTERFACES(WithDock)

public:
    explicit WaveformView(QWidget *parent = nullptr);

private:
    std::array<QSplineSeries *, 6> series = {};
    QChart *chart = nullptr;
    QValueAxis *axisX = nullptr;
    QValueAxis *axisY = nullptr;
    AdcDataModel *adcSampleModel = nullptr;
    QTimer timer;

private slots:
    void updateSeries();
};


#endif //WAVEFORMVIEW_H
