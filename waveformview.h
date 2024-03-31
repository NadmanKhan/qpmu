#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>

class WaveformView : public QChartView
{
    Q_OBJECT
public:
    WaveformView();
};

#endif // WAVEFORMVIEW_H
