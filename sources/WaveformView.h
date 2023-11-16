#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <array>

#include <QtCharts>

#include "App.h"
#include "WithDock.h"

class WaveformView: public QChartView, public WithDock
{
Q_OBJECT

public:
    explicit WaveformView(QWidget *parent = nullptr);

private:
    std::array<QSplineSeries *, 6> series = {};
    QChart *chart = nullptr;
    QValueAxis *axisX = nullptr;
    QValueAxis *axisY = nullptr;

private slots:
    void updateSeries(AdcSampleVector vector,
                      qreal minX,
                      qreal maxX,
                      qreal minY,
                      qreal maxY);
};


#endif //WAVEFORMVIEW_H
