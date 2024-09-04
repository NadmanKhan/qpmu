#ifndef QPMU_APP_MONITOR_VIEW_H
#define QPMU_APP_MONITOR_VIEW_H

#include "qpmu/defs.h"
#include "timeout_notifier.h"
#include "equally_scaled_axes_chart.h"

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

class MonitorView : public QWidget
{
    Q_OBJECT
public:
    static constexpr qreal Spacing = 0.25;
    static constexpr qreal PolarGraphWidth = 2;
    static constexpr qreal RectGraphWidth = 4;

    static constexpr int NumCycles = 1;
    static constexpr int NumPointsPerCycle = 2 * 5;

    static constexpr quint32 UpdateIntervalMs = 100;
    static constexpr std::array<qreal, 6> SimulationFrequencyOptions = {
        0, 0.125, 0.25, 0.5, 1, 2
    };
    static constexpr std::array<qreal, 7> PlotAmpliOptions[2] = {
        { 0, 60, 120, 180, 240, 300, 360 }, { 0, 0.5, 1, 2, 4, 8, 16 }
    };

    static constexpr std::array<char const *const, 3> TableHHeaders = { "Voltage", "Current",
                                                                        "Phase Diff." };

    MonitorView(QWidget *parent = nullptr);

private slots:
    void update(bool force = false);
    void noForceUpdate();
    void forceUpdate();
    bool isSimulating() const;

private:
    TimeoutNotifier *m_updateNotifier;
    TimeoutNotifier *m_simulationUpdateNotifier;
    qpmu::Float m_plotAmplitudes[qpmu::SignalCount];
    qpmu::Float m_plotPhaseDiffs[qpmu::SignalCount];

    QList<QLineSeries *> m_phasorFakeAxesSeriesList[2];
    QList<QLineSeries *> m_waveformFakeAxesSeriesList[2];

    QLineSeries *m_phasorSeriesList[qpmu::SignalCount];
    QLineSeries *m_waveformSeriesList[qpmu::SignalCount];
    QLineSeries *m_connectorSeriesList[qpmu::SignalCount];

    QVector<QPointF> m_phasorPointsList[qpmu::SignalCount];
    QVector<QPointF> m_waveformPointsList[qpmu::SignalCount];
    QVector<QPointF> m_connectorPointsList[qpmu::SignalCount];

    QLabel *m_statusLabel;
    QLabel *m_frequencyLabel;
    QLabel *m_phasorLabels[qpmu::SignalCount];
    QLabel *m_phaseDiffLabels[qpmu::SignalPhaseCount];
    QLabel *m_phasePowerLabels[qpmu::SignalPhaseCount];

    QPushButton *m_pausePlayButton;
    QList<QCheckBox *> m_signalCheckBoxList[2];
    QCheckBox *m_phasorCheckBox;
    QCheckBox *m_waveformCheckBox;
    QCheckBox *m_connectorCheckBox;

    bool m_playing = true;
    QComboBox *m_maxPlotAmplitude[2];
    int m_simulationFrequencyIndex = 0;
};

#endif // QPMU_APP_MONITOR_VIEW_H
