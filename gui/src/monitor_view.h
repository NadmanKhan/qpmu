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
#include <QIcon>
#include <QRadioButton>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <qcombobox.h>
#include <qlineseries.h>

#include "qpmu/common.h"
#include "worker.h"
#include "timeout_notifier.h"
#include "equally_scaled_axes_chart.h"

class MonitorView : public QWidget
{
    Q_OBJECT
public:
    static constexpr qreal Spacing = 0.25;
    static constexpr qreal PolarGraphWidth = 2;
    static constexpr qreal RectGraphWidth = 4;

    static constexpr int NumCycles = 2;
    static constexpr int NumPointsPerCycle = 2 * 5;

    static constexpr quint32 UpdateIntervalMs = 100;
    static constexpr std::array<qreal, 6> SimulationFrequencyOptions = {
        0, 0.125, 0.25, 0.5, 1, 2
    };
    static constexpr std::array<qreal, 7> PlotAmpliOptions[2] = {
        { 0, 60, 120, 180, 240, 300, 360 }, { 0, 0.5, 1, 2, 4, 8, 16 }
    };

    static constexpr char const *const TableHHeaders[] = { "Voltage", "Current", "Phase Diff.",
                                                           "Power" };
    static constexpr int NumTableColumns = 4;

    MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent = nullptr);

private slots:
    void update(bool force = false);
    void noForceUpdate();
    void forceUpdate();

private:
    Worker *m_worker;
    TimeoutNotifier *m_updateNotifier;
    TimeoutNotifier *m_simulationUpdateNotifier;
    qpmu::FloatType m_plotAmplitudes[qpmu::NumChannels];
    qpmu::FloatType m_plotPhaseDiffs[qpmu::NumChannels];

    QList<QLineSeries *> m_phasorFakeAxesSeriesList[2];
    QList<QLineSeries *> m_waveformFakeAxesSeriesList[2];

    QLineSeries *m_phasorSeriesList[qpmu::NumChannels];
    QLineSeries *m_waveformSeriesList[qpmu::NumChannels];
    QLineSeries *m_connectorSeriesList[qpmu::NumChannels];

    QVector<QPointF> m_phasorPointsList[qpmu::NumChannels];
    QVector<QPointF> m_waveformPointsList[qpmu::NumChannels];
    QVector<QPointF> m_connectorPointsList[qpmu::NumChannels];

    QLabel *m_statusLabel;
    QLabel *m_phasorLabels[qpmu::NumChannels];
    QLabel *m_phaseDiffLabels[qpmu::NumPhases];
    QLabel *m_phasePowerLabels[qpmu::NumPhases];

    QPushButton *m_pausePlayButton;
    QList<QCheckBox *> m_signalCheckBoxList[2];
    QCheckBox *m_phasorCheckBox;
    QCheckBox *m_waveformCheckBox;
    QCheckBox *m_connectorCheckBox;

    bool m_playing = true;
    QComboBox *m_maxPlotAmplitude[2];
    int m_simulationFrequencyIndex = 0;
};

#endif // MONITOR_VIEW_H
