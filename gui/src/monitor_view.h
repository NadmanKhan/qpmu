#ifndef MONITOR_VIEW_H
#define MONITOR_VIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>
#include <QTimer>
#include <QBoxLayout>
#include <QColor>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>

#include "qpmu/common.h"
#include "worker.h"
#include "timeout_notifier.h"

class MonitorView : public QWidget
{
    Q_OBJECT
public:
    static constexpr qreal Scale = 5;
    static constexpr qreal Margin = 0.5;
    static constexpr qreal Spacing = 0.5;
    static constexpr qreal PolarGraphWidth = 2;
    static constexpr qreal RectGraphWidth = 4;

    static constexpr quint32 UpdateIntervalMs = 100;
    static constexpr qreal SimulationFrequencyOptions[] = { 2, 1, 0.5, 0.25, 0.125 };

    static constexpr int NumCycles = 2;
    static constexpr int NumPointsPerCycle = 18;

    MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent = nullptr);

private slots:
    void update();

private:
    Worker *m_worker;

    qpmu::ComplexType m_plottedPhasors[qpmu::NumChannels];

    QLineSeries *m_phasorSeriesList[qpmu::NumChannels];
    QLineSeries *m_waveformSeriesList[qpmu::NumChannels];
    QLineSeries *m_connectorSeriesList[qpmu::NumChannels];

    QVector<QPointF> m_phasorPointsList[qpmu::NumChannels];
    QVector<QPointF> m_waveformPointsList[qpmu::NumChannels];
    QVector<QPointF> m_connectorPointsList[qpmu::NumChannels];

    TimeoutNotifier *m_updateNotifier;

    QComboBox *m_comboSimulationFrequency;
};

#endif // MONITOR_VIEW_H
