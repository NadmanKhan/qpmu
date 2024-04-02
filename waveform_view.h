#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

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

#endif // WAVEFORM_VIEW_H
