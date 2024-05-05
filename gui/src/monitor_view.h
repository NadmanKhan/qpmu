#ifndef MONITOR_VIEW_H
#define MONITOR_VIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QColor>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>

#include "qpmu/common.h"
#include "worker.h"
#include "timeout_notifier.h"

class MonitorView : public QWidget
{
    Q_OBJECT
public:
    static constexpr qreal Scale = 5;
    static constexpr qreal Spacing = 0.25;
    static constexpr qreal PolarGraphWidth = 2;
    static constexpr qreal RectGraphWidth = 4;

    static constexpr int NumCycles = 2;
    static constexpr int NumPointsPerCycle = 2 * 35;

    static constexpr quint32 UpdateIntervalMs = 200;
    static constexpr qreal SimulationFrequencyOptions[] = { 0, 0.125, 0.25, 0.5, 1, 2 };
    static constexpr qreal VoltagePlotRanges[] = { 0, 60, 120, 180, 240, 300, 360 };
    static constexpr qreal CurrentPlotRanges[] = { 0, 1, 2, 10, 20, 100, 200 };

    MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent = nullptr);

private slots:
    void update();

private:
    Worker *m_worker;
    TimeoutNotifier *m_updateNotifier;

    qpmu::ComplexType m_plottedPhasors[qpmu::NumChannels];

    QLineSeries *m_phasorSeriesList[qpmu::NumChannels];
    QLineSeries *m_waveformSeriesList[qpmu::NumChannels];
    QLineSeries *m_connectorSeriesList[qpmu::NumChannels];

    QVector<QPointF> m_phasorPointsList[qpmu::NumChannels];
    QVector<QPointF> m_waveformPointsList[qpmu::NumChannels];
    QVector<QPointF> m_connectorPointsList[qpmu::NumChannels];

    int m_simulationFrequencyIndex = 0;
    int m_plotRangeIndex[2] = { 0, 0 };
};

#endif // MONITOR_VIEW_H
