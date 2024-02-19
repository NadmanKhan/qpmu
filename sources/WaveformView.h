#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <array>

#include <QtCharts>
#include <QTimer>

#include "App.h"
#include "SignalInfoModel.h"

class WaveformView: public QChartView
{
Q_OBJECT

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
