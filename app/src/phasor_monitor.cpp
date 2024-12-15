#include "qpmu/defs.h"
#include "app.h"
#include "data_processor.h"
#include "equally_scaled_axes_chart.h"
#include "phasor_monitor.h"
#include "settings_models.h"
#include "src/main_page_interface.h"
#include "util.h"

#include <QTableWidget>
#include <QTableWidget>
#include <QAbstractSeries>
#include <QBoxLayout>
#include <QtGlobal>
#include <QLineSeries>
#include <QMargins>
#include <QIcon>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QSizePolicy>
#include <qnamespace.h>
#include <qwidget.h>

using namespace qpmu;
#define QSL(s) QStringLiteral(s)
#define FMT_FIELD(value, unit) (QSL("<pre><b>%1</b><small>%2</small></pre>").arg(value, unit))
#define FMT_VALUE(value) (QString::number(value, 'f', 1).rightJustified(6))

constexpr Float Margin = 0.025;
constexpr Float PhasorPlotWidth = 2;
constexpr Float WaveformPlotWidth = 4;
constexpr Float InterPlotSpace = 0.25;
constexpr ssize_t CountCycles = 1;
constexpr ssize_t CountPointsPerCycle = 2 * 5;

enum PlotState { Joint = 0, Alone = 1 };

/// Offset for the phasor plot's origin, i.e., the (0, 0) point of the polar graph
constexpr Float PhasorOffsets[2] = {
    /// Both plots present -> center phasor plot area
    PhasorPlotWidth / 2,
    /// Only itself present -> remain in the center
    (PhasorPlotWidth + InterPlotSpace + WaveformPlotWidth) / 2
};

Float diffAnglesRad(Float a, Float b)
{
    auto diff = a - b;
    if (diff < -M_PI) {
        diff += 2 * M_PI;
    } else if (diff > M_PI) {
        diff -= 2 * M_PI;
    }
    return diff;
}

Float diffAnglesDeg(Float a, Float b)
{
    auto diff = a - b;
    if (diff < -180) {
        diff += 2 * 180;
    } else if (diff > 180) {
        diff -= 2 * 180;
    }
    return diff;
}

/// Offset for the waveform plot's origin, i.e., the (0, 0) point of the rectangluar graph
constexpr Float WaveformOffsets[2] = {
    /// Both plots present -> start at the waveform plot area
    PhasorPlotWidth + InterPlotSpace,
    /// Only itself present -> remain in the center
    (PhasorPlotWidth + InterPlotSpace + WaveformPlotWidth) / 2 - WaveformPlotWidth / 2
};

PhasorMonitor::PhasorMonitor(QWidget *parent) : QWidget(parent)
{
    hide();

    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(0, 0, 0, 0);

    createChartView();
    createSummaryBar();
    createTable();
    createControls();
    updateVisibility();

    connect(APP->timer(), &QTimer::timeout, this, &PhasorMonitor::updateView);
}

void PhasorMonitor::updateView()
{
    if (!isVisible()) {
        return;
    }

    const auto est = APP->dataProcessor()->currentEstimation();
    const auto samples = APP->dataProcessor()->currentSampleStore();

    { /// To start, update colors of series and labels

        VisualisationSettings visualSettings;
        visualSettings.load();

        for (uint64_t i = 0; i < CountSignals; ++i) {
            const auto &color = visualSettings.signalColors[i];
            for (auto colorLabel : m_colorLabels[i]) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                auto pixmap = colorLabel->pixmap();
#else
                auto pixmap = *colorLabel->pixmap();
#endif
                colorLabel->setPixmap(rectPixmap(color, pixmap.width(), pixmap.height()));
            }
            for (PlotState state : { Joint, Alone }) {
                auto phasorSeries = m_seriesList[state].phasors[i];
                auto waveformSeries = m_seriesList[state].waveforms[i];
                auto connectorSeries = m_seriesList[state].connectors[i];
                phasorSeries->setColor(color);
                waveformSeries->setColor(color);
                connectorSeries->setColor(color);
            }
        }
        for (SignalType type : { VoltageSignal, CurrentSignal }) {
            auto currentTypedSignals = SignalsOfType[type];
            for (uint64_t i = 0; i < CountSignalPhases; ++i) {
                auto color = visualSettings.signalColors[currentTypedSignals[i]];
                auto newPixmap = circlePixmap(color, 10);
                m_ctrl.visibility.checkSignalsOfType[type][i]->setIcon(QIcon(newPixmap));
            }
        }
    }

    if (!m_playing) {
        /// Paused, so do not update data
        return;
    }

    CalibrationSettings calibSettings;
    calibSettings.load();

    /// Get the amplitudes and phases of the phasors
    Float amplis[CountSignals];
    Float phasesRad[CountSignals];
    Float phasesDeg[CountSignals];
    for (uint64_t i = 0; i < CountSignals; ++i) {
        const auto &calibData = calibSettings.data[i];
        const auto &phasor = est.phasors[i];
        auto ampli = calibData.slope * std::abs(phasor) + calibData.intercept;
        auto phaseRad = std::arg(phasor);
        auto phaseDeg = phaseRad * 180 / M_PI; /// Convert to degrees

        amplis[i] = ampli;
        phasesRad[i] = phaseRad;
        phasesDeg[i] = phaseDeg;
    }

    /// Get the amplitude scale values
    Float vMaxAmpli = m_ctrl.ampliScale.voltage->currentData().toFloat();
    if (m_ctrl.ampliScale.voltage->currentIndex() == 0) {
        for (auto signalId : SignalsOfType[VoltageSignal]) {
            vMaxAmpli = std::max(vMaxAmpli, amplis[signalId]);
        }
    }
    Float iMaxAmpli = m_ctrl.ampliScale.current->currentData().toFloat();
    if (m_ctrl.ampliScale.current->currentIndex() == 0) {
        for (auto signalId : SignalsOfType[CurrentSignal]) {
            iMaxAmpli = std::max(iMaxAmpli, amplis[signalId]);
        };
    }

    /// Get the phase reference ids
    int vPhaseRefId = m_ctrl.phaseRef.voltage->currentData().toInt();
    int iPhaseRefId = m_ctrl.phaseRef.current->currentData().toInt();

    /// Calculate the normalized amplitudes and phases
    Float normAmplis[CountSignals];
    Float normPhasesRad[CountSignals];
    Float normPhasesDeg[CountSignals];
    bool ampliExceedsPlot = false;
    for (uint64_t i = 0; i < CountSignals; ++i) {
        /// Normalized amplitude
        auto ampliNorm = amplis[i];
        if (TypeOfSignal[i] == VoltageSignal) {
            ampliNorm /= vMaxAmpli;
        } else {
            ampliNorm /= iMaxAmpli;
        }
        ampliExceedsPlot = ampliNorm > 1 + 1e-9;

        /// Normalized phase; DO NOT FORGET TO CONVERT TO RADIANS
        auto phaseNormRad = phasesRad[i];
        if (TypeOfSignal[i] == VoltageSignal) {
            if (vPhaseRefId != -1) {
                phaseNormRad = diffAnglesRad(phaseNormRad, phasesRad[vPhaseRefId]);
            }
        } else {
            if (iPhaseRefId != -1) {
                phaseNormRad = diffAnglesRad(phaseNormRad, phasesRad[iPhaseRefId]);
            }
        }
        auto phaseNormDeg = phaseNormRad * 180 / M_PI; /// Convert to  degrees

        normAmplis[i] = ampliNorm;
        normPhasesRad[i] = phaseNormRad;
        normPhasesDeg[i] = phaseNormDeg;
    }

    for (uint64_t i = 0; i < CountSignals; ++i) {
        /// Update series points

        auto ampli = normAmplis[i];
        auto phase = normPhasesRad[i];

        for (PlotState state : { Joint, Alone }) {
            auto &phasorPoints = m_seriesPointList[state].phasors[i];
            auto &waveformPoints = m_seriesPointList[state].waveforms[i];
            auto &connectorPoints = m_seriesPointList[state].connectors[i];

            { /// Update phasor points

                /// Phasor series path: origin > phasor > arrow-left-arm > phasor >
                /// arrow-right-arm. Alpha = angle that the each of the two arrow arms makes
                /// with the phasor line at the origin

                if (ampliExceedsPlot) {
                    phasorPoints[0] = QPointF(0, 0);
                    phasorPoints[1] = unitvector(phase);
                    phasorPoints[2] = phasorPoints[0];
                    phasorPoints[3] = phasorPoints[1];
                    phasorPoints[4] = phasorPoints[0];
                } else {
                    Float alpha;
                    Float alphaHypotenuse;
                    {
                        Float alphaOpposite = 0.03;
                        Float alphaAdjacent = ampli - (2 * alphaOpposite);
                        alpha = std::atan(alphaOpposite / alphaAdjacent);
                        alphaHypotenuse = std::sqrt(alphaOpposite * alphaOpposite
                                                    + alphaAdjacent * alphaAdjacent);
                    }
                    phasorPoints[0] = QPointF(0, 0);
                    phasorPoints[1] = ampli * unitvector(phase);
                    phasorPoints[2] = alphaHypotenuse * unitvector(phase + alpha);
                    phasorPoints[3] = phasorPoints[1];
                    phasorPoints[4] = alphaHypotenuse * unitvector(phase - alpha);
                }
            }

            { /// Update waveform points

                for (uint64_t j = 0; j < (CountCycles * CountPointsPerCycle + 1); ++j) {
                    Float t = j * (1.0 / CountPointsPerCycle);
                    Float y = ampli * std::sin(-2 * M_PI * t + phase);
                    Float x = j * (WaveformPlotWidth / (CountCycles * CountPointsPerCycle));
                    waveformPoints[j] = QPointF(x, y);
                }
            }

            { /// Translate by the appropriate offset

                for (auto &point : phasorPoints) {
                    point.setX(point.x() + PhasorOffsets[state]);
                }
                for (auto &point : waveformPoints) {
                    point.setX(point.x() + WaveformOffsets[state]);
                }
            }

            { /// Update connector points

                if (ampliExceedsPlot) {
                    connectorPoints[0] = connectorPoints[1] = phasorPoints[1];
                } else {
                    connectorPoints[0] = phasorPoints[1];
                    connectorPoints[1] = waveformPoints[0];
                }
            }

            { /// Commit points
                auto phasorSeries = m_seriesList[state].phasors[i];
                auto waveformSeries = m_seriesList[state].waveforms[i];
                auto connectorSeries = m_seriesList[state].connectors[i];
                phasorSeries->replace(phasorPoints);
                waveformSeries->replace(waveformPoints);
                connectorSeries->replace(connectorPoints);
            }
        }
    }

    { /// Update data labels

        for (uint64_t i = 0; i < CountSignals; ++i) {
            const Float &frequ = est.frequencies[i];
            const Float &ampli = amplis[i];
            const Float &phase = m_ctrl.phaseRef.checkApplyToTable->isChecked() ? normPhasesDeg[i]
                                                                                : phasesDeg[i];

            auto ampliUnit = UnitSymbolOfSignalType[TypeOfSignal[i]];
            m_labels.ampli[i]->setText(FMT_FIELD(FMT_VALUE(ampli), QSL(" %1").arg(ampliUnit)));
            m_labels.phase[i]->setText(FMT_FIELD(FMT_VALUE(phase), QSL("°")));
            // m_labels.frequ[i]->setText(FMT_FIELD(FMT_VALUE(frequ), QSL(" Hz")));
        }

        for (uint64_t p = 0; p < CountSignalPhases; ++p) {
            const auto &[vId, iId] = SignalsOfPhase[p];
            const auto vPhaseDeg = phasesDeg[vId];
            const auto iPhaseDeg = phasesDeg[iId];
            auto phaseDiff = diffAnglesDeg(vPhaseDeg, iPhaseDeg);

            m_labels.phaseDiff[p]->setText(FMT_FIELD(FMT_VALUE(phaseDiff), QSL("°")));
        }

        m_labels.summaryFrequency->setText(FMT_FIELD(FMT_VALUE(est.frequencies[0]), QSL(" Hz")));
        m_labels.summarySamplingRate->setText(
                FMT_FIELD(FMT_VALUE(est.samplingRate), QSL(" samples/s")));
        m_labels.summaryLastSampleTime->setText(
                QDateTime::fromMSecsSinceEpoch(samples.back().timestampUsec / 1000)
                        .toString(QSL("hh:mm:ss.zzz")));
    }
}

void PhasorMonitor::updateVisibility()
{

    auto bucket = m_ctrl.visibility.bucket;
    auto signalsOfType = m_ctrl.visibility.checkSignalsOfType;
    auto phasors = m_ctrl.visibility.checkPhasors;
    auto waveforms = m_ctrl.visibility.checkWaveforms;
    auto connectors = m_ctrl.visibility.checkConnectors;

#define SIGNAL_CHECKED(i) (signalsOfType[TypeOfSignal[(i)]][PhaseOfSignal[(i)]]->isChecked())
#define SIGNAL_TYPE_CHECKED(i) (signalsOfType[TypeOfSignal[(i)]][3]->isChecked())

    if (!phasors->isChecked() && !waveforms->isChecked()) {
        /// Hide everything
        for (PlotState state : { Joint, Alone }) {
            bucket[0].append(m_fakeAxesSeriesList[state].phasor);
            bucket[0].append(m_fakeAxesSeriesList[state].waveform);
            for (uint64_t i = 0; i < CountSignals; ++i) {
                bucket[0].append(m_seriesList[state].phasors[i]);
                bucket[0].append(m_seriesList[state].waveforms[i]);
                bucket[0].append(m_seriesList[state].connectors[i]);
            }
        }

    } else if (!phasors->isChecked() && waveforms->isChecked()) {
        /// Hide phasors and connectors, show waveforms
        for (PlotState state : { Joint, Alone }) {
            bucket[0].append(m_fakeAxesSeriesList[state].phasor);
            bucket[state == Alone].append(m_fakeAxesSeriesList[state].waveform);

            for (uint64_t i = 0; i < CountSignals; ++i) {
                bucket[0].append(m_seriesList[state].phasors[i]);
                bucket[0].append(m_seriesList[state].connectors[i]);

                auto series = m_seriesList[state].waveforms[i];
                bucket[state == Alone && SIGNAL_CHECKED(i) && SIGNAL_TYPE_CHECKED(i)].append(
                        series);
            }
        }

    } else if (phasors->isChecked() && !waveforms->isChecked()) {
        /// Show phasors, hide waveforms and connectors
        for (PlotState state : { Joint, Alone }) {
            bucket[state == Alone].append(m_fakeAxesSeriesList[state].phasor);
            bucket[0].append(m_fakeAxesSeriesList[state].waveform);

            for (uint64_t i = 0; i < CountSignals; ++i) {
                auto series = m_seriesList[state].phasors[i];
                bucket[state == Alone && SIGNAL_CHECKED(i) && SIGNAL_TYPE_CHECKED(i)].append(
                        series);

                bucket[0].append(m_seriesList[state].waveforms[i]);
                bucket[0].append(m_seriesList[state].connectors[i]);
            }
        }

    } else {
        /// Show everything of state Joint, hide everything of state Alone
        for (PlotState state : { Joint, Alone }) {
            bucket[state == Joint].append(m_fakeAxesSeriesList[state].phasor);
            bucket[state == Joint].append(m_fakeAxesSeriesList[state].waveform);

            for (uint64_t i = 0; i < CountSignals; ++i) {
                const auto &sl = m_seriesList[state];
                for (auto series : { sl.phasors[i], sl.waveforms[i], sl.connectors[i] }) {
                    bucket[state == Joint && SIGNAL_CHECKED(i) && SIGNAL_TYPE_CHECKED(i)
                           && ((series != sl.connectors[i]) || connectors->isChecked())]
                            .append(series);
                }
            }
        }
    }

    for (bool visibility : { 0, 1 }) {
        for (auto series : bucket[visibility]) {
            series->setVisible(visibility);
        }
    }

#undef SIGNAL_CHECKED
#undef SIGNAL_TYPE_CHECKED

    bucket[0].clear();
    bucket[1].clear();
}

void PhasorMonitor::createControls()
{
    VisualisationSettings visualSettings;
    visualSettings.load();

    m_ctrl.visibility.box = new QGroupBox(this);
    m_ctrl.phaseRef.box = new QGroupBox(this);
    m_ctrl.ampliScale.box = new QGroupBox(this);

    for (auto box : { m_ctrl.visibility.box, m_ctrl.ampliScale.box, m_ctrl.phaseRef.box }) {
        box->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    }

    { /// Visibility controls

        auto layout = new QVBoxLayout(m_ctrl.visibility.box);

        QVector<QCheckBox *> checkBoxes;

        for (uint64_t i = 0; i < CountSignalTypes; ++i) {
            auto nameOfType = NameOfSignalType[TypeOfSignal[i]];
            auto signalOfPhase = SignalsOfType[i];

            /// A check-box for of the 3 signals (one per phase) of the type
            for (uint64_t j = 0; j < CountSignalPhases; ++j) {
                auto checkBox = new QCheckBox(QString(NameOfSignal[signalOfPhase[j]]),
                                              m_ctrl.visibility.box);
                m_ctrl.visibility.checkSignalsOfType[i][j] = checkBox;

                auto icon = QIcon(circlePixmap(visualSettings.signalColors[signalOfPhase[j]], 10));
                checkBox->setIcon(icon);

                checkBoxes << checkBox;
                layout->addWidget(checkBox);
            }

            /// A check-box for all the 3 signals of this type
            auto allCheckBox = new QCheckBox(QSL("All %1s").arg(nameOfType));
            m_ctrl.visibility.checkSignalsOfType[i][3] = allCheckBox;
            checkBoxes << allCheckBox;
            layout->addWidget(allCheckBox);

            { /// Add a separator
                auto hline = new QFrame(m_ctrl.visibility.box);
                hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
                layout->addWidget(hline);
            }
        }

        m_ctrl.visibility.checkPhasors = new QCheckBox(QSL("Phasors"));
        m_ctrl.visibility.checkWaveforms = new QCheckBox(QSL("Waveforms"));
        m_ctrl.visibility.checkConnectors = new QCheckBox(QSL("Connectors"));

        m_ctrl.visibility.checkPhasors->setIcon(QIcon(QSL(":/vector.png")));
        m_ctrl.visibility.checkWaveforms->setIcon(QIcon(QSL(":/waves.png")));
        m_ctrl.visibility.checkConnectors->setIcon(QIcon(QSL(":/dashed-line.png")));

        layout->addWidget(m_ctrl.visibility.checkPhasors);
        layout->addWidget(m_ctrl.visibility.checkWaveforms);
        layout->addWidget(m_ctrl.visibility.checkConnectors);

        checkBoxes << m_ctrl.visibility.checkPhasors;
        checkBoxes << m_ctrl.visibility.checkWaveforms;
        checkBoxes << m_ctrl.visibility.checkConnectors;

        /// Preallocate the `bucket` list
        auto totalSeriesCount = 2
                * (m_fakeAxesSeriesList[0].phasor.size() + m_fakeAxesSeriesList[0].waveform.size()
                   + (3 * CountSignals));

        m_ctrl.visibility.bucket[0].reserve(totalSeriesCount);
        m_ctrl.visibility.bucket[1].reserve(totalSeriesCount);

        for (auto checkBox : checkBoxes) {
            checkBox->setChecked(true);
            connect(checkBox, &QCheckBox::toggled, this, &PhasorMonitor::updateVisibility);
        }
    }

    { /// Phase ref

        m_ctrl.phaseRef.voltage = new QComboBox(m_ctrl.phaseRef.box);
        m_ctrl.phaseRef.current = new QComboBox(m_ctrl.phaseRef.box);

        auto formLayout = new QFormLayout(m_ctrl.phaseRef.box);
        formLayout->addRow("Voltage", m_ctrl.phaseRef.voltage);
        formLayout->addRow("Current", m_ctrl.phaseRef.current);

        m_ctrl.phaseRef.voltage->addItem("None", -1);
        m_ctrl.phaseRef.current->addItem("None", -1);
        for (uint64_t i = 0; i < CountSignals; ++i) {
            m_ctrl.phaseRef.voltage->addItem(NameOfSignal[i], SignalId[i]);
            m_ctrl.phaseRef.current->addItem(NameOfSignal[i], SignalId[i]);
        }

        m_ctrl.phaseRef.voltage->setCurrentIndex(1);
        m_ctrl.phaseRef.current->setCurrentIndex(1);

        m_ctrl.phaseRef.checkApplyToTable = new QCheckBox(QSL("Apply to table"));
        formLayout->addRow(m_ctrl.phaseRef.checkApplyToTable);
        m_ctrl.phaseRef.checkApplyToTable->setChecked(true);
    }

    { /// Amplitude ref

        m_ctrl.ampliScale.voltage = new QComboBox(m_ctrl.ampliScale.box);
        m_ctrl.ampliScale.current = new QComboBox(m_ctrl.ampliScale.box);

        auto formLayout = new QFormLayout(m_ctrl.ampliScale.box);
        formLayout->addRow("Voltage", m_ctrl.ampliScale.voltage);
        formLayout->addRow("Current", m_ctrl.ampliScale.current);

        m_ctrl.ampliScale.voltage->addItem("Auto", 0);
        for (Float v : { 60.0, 120.0, 180.0, 240.0, 300.0, 360.0 }) {
            m_ctrl.ampliScale.voltage->addItem(
                    QSL("%1 %2").arg(v).arg(UnitSymbolOfSignalType[VoltageSignal]), v);
        }

        m_ctrl.ampliScale.current->addItem("Auto", 0);
        for (Float i : { 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 }) {
            m_ctrl.ampliScale.current->addItem(
                    QSL("%1 %2").arg(i).arg(UnitSymbolOfSignalType[CurrentSignal]), i);
        }

        m_ctrl.ampliScale.voltage->setCurrentIndex(0);
        m_ctrl.ampliScale.current->setCurrentIndex(0);
    }
}

void PhasorMonitor::createChartView()
{
    VisualisationSettings visualSettings;
    visualSettings.load();

    auto chartView = new QChartView(this);
    m_outerLayout->addWidget(chartView, 100);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setContentsMargins(QMargins(0, 0, 0, 0));
    chartView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    chartView->setBackgroundRole(QPalette::Base);
    chartView->setAutoFillBackground(true);

    auto chart = new EquallyScaledAxesChart();
    { /// Chart
        chartView->setChart(chart);
        chart->setMargins(QMargins(0, 0, 0, 0));
        chart->setBackgroundRoundness(0);
        chart->legend()->hide();
        chart->setAnimationOptions(QChart::NoAnimation);
    }

    auto xAxis = new QValueAxis();
    auto yAxis = new QValueAxis();
    { /// Axes
        chart->addAxis(xAxis, Qt::AlignBottom);
        chart->addAxis(yAxis, Qt::AlignLeft);

        xAxis->setRange(-Margin, (PhasorPlotWidth + InterPlotSpace + WaveformPlotWidth) + Margin);
        yAxis->setRange(-(Margin + PhasorPlotWidth / 2), +(Margin + PhasorPlotWidth / 2));

        for (auto axis : { xAxis, yAxis }) {
            axis->setLabelsVisible(false);
            axis->setTickCount(2);
            axis->setVisible(false);
        }
    }

    /// Fake axes
    /// 1. Create the axes assuming they center at the origin (0, 0)
    /// 2. Translate the axes to the desired position for the two scenarios:
    ///    a. Both phasor and waveform are present (index 0 of series list)
    ///    b. Only one of them is present (index 1 of series list)

    /// Create a and return new fake axis series, adding it to the chart
    auto makeFakeAxisSeries = [=](QAbstractSeries::SeriesType type, Float penWidth) {
        QLineSeries *s = nullptr;
        if (type == QAbstractSeries::SeriesTypeSpline) {
            s = new QSplineSeries();
        } else if (type == QAbstractSeries::SeriesTypeLine) {
            s = new QLineSeries();
        }
        Q_ASSERT(s);
        auto color = QColor("lightGray");
        s->setPen(QPen(color, penWidth));
        chart->addSeries(s);
        s->attachAxis(xAxis);
        s->attachAxis(yAxis);
        s->setVisible(false);
        return s;
    };

    { /// Phasor fake axes
        for (PlotState state : { Joint, Alone }) {
            /// Radial axis series list (circles)
            for (Float radiusPercent : { 100, 89, 67 }) {
                const auto radius = (PhasorPlotWidth / 2) * (radiusPercent / 100);
                auto radialAxis = makeFakeAxisSeries(QAbstractSeries::SeriesTypeSpline, 1.5);

                constexpr ssize_t thetaStep = 10;
                for (ssize_t i = 0; i <= (360 / thetaStep); ++i) {
                    const auto theta = (i * thetaStep) * (2 * M_PI / 360);
                    radialAxis->append(radius * unitvector(theta));
                }

                m_fakeAxesSeriesList[state].phasor.append(radialAxis);
            }

            /// Angular axis series list (tick lines)
            constexpr ssize_t thetaStep = 30;
            for (ssize_t i = 0; i <= (360 / thetaStep); ++i) {
                const auto theta = (i * thetaStep) * (2 * M_PI / 360);
                auto angularAxis = makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 1);
                angularAxis->append(0, 0);
                angularAxis->append((1 + Margin) * unitvector(theta));

                m_fakeAxesSeriesList[state].phasor.append(angularAxis);
            }

            /// Translate the axes to the desired positions
            for (auto series : m_fakeAxesSeriesList[state].phasor) {
                for (ssize_t i = 0; i < series->count(); ++i) {
                    series->replace(i, series->at(i) + QPointF(PhasorOffsets[state], 0));
                }
            }
        }
    }

    { /// Waveform fake axes
        for (PlotState state : { Joint, Alone }) {
            /// Vertical axis
            auto verticalAxis = makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 2);
            verticalAxis->append(0, -(PhasorPlotWidth / 2));
            verticalAxis->append(0, +(PhasorPlotWidth / 2));

            /// Horizontal axis
            auto horizontalAxis = makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 2);
            horizontalAxis->append(0, 0);
            horizontalAxis->append(WaveformPlotWidth, 0);

            m_fakeAxesSeriesList[state].waveform.append(verticalAxis);
            m_fakeAxesSeriesList[state].waveform.append(horizontalAxis);

            for (auto series : m_fakeAxesSeriesList[state].waveform) {
                for (ssize_t i = 0; i < series->count(); ++i) {
                    series->replace(i, series->at(i) + QPointF(WaveformOffsets[state], 0));
                }
            }
        }
    }

    {
        for (auto series : m_fakeAxesSeriesList[0].phasor) {
            series->setVisible(true);
        }
        for (auto series : m_fakeAxesSeriesList[1].phasor) {
            series->setVisible(false);
        }

        for (auto series : m_fakeAxesSeriesList[0].waveform) {
            series->setVisible(true);
        }
        for (auto series : m_fakeAxesSeriesList[1].waveform) {
            series->setVisible(false);
        }
    }

    /// Series list

    for (PlotState state : { Joint, Alone }) {
        for (uint64_t i = 0; i < CountSignals; ++i) {
            auto name = QString(NameOfSignal[i]);
            auto color = visualSettings.signalColors[i];

            QLineSeries *phasorSeries = new QLineSeries();
            QLineSeries *waveformSeries = new QSplineSeries();
            QLineSeries *connectorSeries = new QLineSeries();

            auto phasorPen = QPen(color, 3, Qt::SolidLine);
            auto waveformPen = QPen(color, 2, Qt::SolidLine);
            auto connectorPen = QPen(color, 1, Qt::DotLine);

            phasorSeries->setPen(phasorPen);
            waveformSeries->setPen(waveformPen);
            connectorSeries->setPen(connectorPen);

            m_seriesList[state].phasors[i] = phasorSeries;
            m_seriesList[state].waveforms[i] = waveformSeries;
            m_seriesList[state].connectors[i] = connectorSeries;

            m_seriesPointList[state].phasors[i] = PointVector(5);
            m_seriesPointList[state].waveforms[i] =
                    PointVector(CountCycles * CountPointsPerCycle + 1);
            m_seriesPointList[state].connectors[i] = PointVector(2);

            for (auto series : { phasorSeries, waveformSeries, connectorSeries }) {
                series->setName(name);
                series->setVisible(true);
                chart->addSeries(series);
                series->attachAxis(xAxis);
                series->attachAxis(yAxis);
            }
        }
    }
}

void PhasorMonitor::createTable()
{
    /// 3 rows: one for each phase
    /// 3 columns: voltage (phasor and frequency), current (phasor and frequency), phase
    /// difference

    VisualisationSettings visualSettings;
    visualSettings.load();

    auto table = new QTableWidget(this);
    m_outerLayout->addWidget(table, 1);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({ "Voltage", "Current", "ΔΦ" });
    table->setRowCount(3);
    table->setVerticalHeaderLabels({ "A", "B", "C" });

    auto makeCellWidget = [=]() -> QWidget * {
        auto widget = new QWidget(table);
        widget->setFont(QFont("Monospace", APP->font().pointSize()));
        auto layout = new QHBoxLayout(widget);
        layout->setContentsMargins(QMargins(0, 0, 0, 0));
        layout->setSpacing(0);
        layout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        return widget;
    };

    auto makeDataLabel = [=](QWidget *parent) -> QLabel * {
        auto label = new QLabel(QSL("_"), parent);
        label->setContentsMargins(QMargins(0, 0, 0, 0));
        label->raise();
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        label->setTextFormat(Qt::RichText);
        auto font = label->font();
        font.setFamily("monospace");
        label->setFont(font);
        return label;
    };

    auto makeDecorativeLabel = [=](QWidget *parent) -> QLabel * {
        auto label = new QLabel(parent);
        label->setContentsMargins(QMargins(0, 0, 0, 0));
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setTextFormat(Qt::RichText);
        label->setScaledContents(true);
        auto font = label->font();
        font.setWeight(QFont::Thin);
        label->setFont(font);
        return label;
    };

    auto makePhasorCellWidget = [=](Signal signalId) -> QWidget * {
        auto widget = makeCellWidget();
        auto layout = static_cast<QHBoxLayout *>(widget->layout());

        QLabel *colorLabel; // e.g. color square
        QLabel *ampliLabel; // e.g. "120.0"
        QLabel *phaseLabel; // e.g. "30.0"
        // QLabel *frequLabel; // e.g. "50.0"

        layout->addWidget(colorLabel = makeDecorativeLabel(widget));
        layout->addWidget(ampliLabel = makeDataLabel(widget), 1);
        layout->addWidget(phaseLabel = makeDataLabel(widget), 1);
        // layout->addWidget(frequLabel = makeDataLabel(widget), 1);

        colorLabel->setPixmap(
                rectPixmap(visualSettings.signalColors[signalId], 6, 0.7 * colorLabel->height()));
        colorLabel->setScaledContents(false);

        m_colorLabels[signalId].append(colorLabel);
        m_labels.ampli[signalId] = ampliLabel;
        m_labels.phase[signalId] = phaseLabel;
        // m_labels.frequ[signalId] = frequLabel;

        return widget;
    };

    /// Cell widgets
    for (uint64_t phase = 0; phase < CountSignalPhases; ++phase) {
        auto phaseDiffWidget = makeCellWidget();
        { /// Phase difference cell
            auto layout = static_cast<QHBoxLayout *>(phaseDiffWidget->layout());

            QLabel *phaseDiffLabel; // e.g. "15.0"

            layout->addWidget(phaseDiffLabel = makeDataLabel(phaseDiffWidget), 1);

            m_labels.phaseDiff[phase] = phaseDiffLabel;
        }

        const auto &[vId, iId] = SignalsOfPhase[phase];

        table->setCellWidget(phase, 0, makePhasorCellWidget(vId));
        table->setCellWidget(phase, 1, makePhasorCellWidget(iId));
        table->setCellWidget(phase, 2, phaseDiffWidget);
    }

    { /// Fix table dimensions and alignment
        int tableHeight = 0;
        for (int i = 0; i < table->rowCount(); ++i) {
            table->resizeRowToContents(i);
            auto h = table->rowHeight(i);
            table->setRowHeight(i, h);
            tableHeight += table->rowHeight(i);
        }
        tableHeight += table->horizontalHeader()->height();
        table->setFixedHeight(tableHeight);
        table->setMinimumHeight(tableHeight);
        table->setMaximumHeight(tableHeight);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
        table->horizontalHeader()->resizeSection(2, table->font().pointSize() * 7);
    }
}

void PhasorMonitor::createSummaryBar()
{
    /// Use a layout layout to make summary bar
    auto summaryLayout = new QHBoxLayout();
    m_outerLayout->addLayout(summaryLayout);
    summaryLayout->setContentsMargins(QMargins(0, 0, 0, 0));

    /// Initialize and add widgets to layout
    auto pausePlayButton = new QPushButton(this);
    auto summaryLiveStatus = new QLabel(this);
    m_labels.summaryFrequency = new QLabel(this);
    m_labels.summarySamplingRate = new QLabel(this);
    m_labels.summaryLastSampleTime = new QLabel(this);
    summaryLayout->addWidget(pausePlayButton);
    summaryLayout->addWidget(summaryLiveStatus);
    summaryLayout->addWidget(m_labels.summaryLastSampleTime, 1);
    summaryLayout->addWidget(m_labels.summarySamplingRate);
    summaryLayout->addWidget(m_labels.summaryFrequency);

    /// Set styles
    for (auto widget :
         { (QWidget *)pausePlayButton, (QWidget *)summaryLiveStatus,
           (QWidget *)m_labels.summaryFrequency, (QWidget *)m_labels.summarySamplingRate,
           (QWidget *)m_labels.summaryLastSampleTime }) {
        widget->setAutoFillBackground(true);
        widget->setBackgroundRole(QPalette::Highlight);
        widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        if (auto label = qobject_cast<QLabel *>(widget)) {
            label->setForegroundRole(QPalette::BrightText);
            label->setAlignment(Qt::AlignVCenter);
            label->setTextFormat(Qt::RichText);
            label->setContentsMargins(QMargins(10, 5, 10, 5));
            auto font = label->font();
            font.setPointSize(font.pointSize() * 1.25);
            font.setBold(true);
            label->setFont(font);
        }
    }

    { /// Pause/play button and LIVE/PAUSED label

        auto pauseIcon = QIcon(":/pause.png");
        auto playIcon = QIcon(":/play.png");
        auto livePalette = summaryLiveStatus->palette();
        auto pausedPalette = summaryLiveStatus->palette();
        livePalette.setColor(QPalette::Highlight, QSL("#22bb45"));
        pausedPalette.setColor(QPalette::Highlight, QSL("#dc143c"));

        /// Set initial state: button = playing
        m_playing = true;
        pausePlayButton->setIcon(pauseIcon);

        /// Set initial state: status = LIVE
        summaryLiveStatus->setText(QSL("LIVE"));
        summaryLiveStatus->setPalette(livePalette);

        connect(pausePlayButton, &QPushButton::clicked, [=] {
            m_playing = !m_playing;
            if (m_playing) {
                pausePlayButton->setIcon(pauseIcon);
                summaryLiveStatus->setText(QSL("LIVE"));
                summaryLiveStatus->setPalette(livePalette);
            } else {
                pausePlayButton->setIcon(playIcon);
                summaryLiveStatus->setText(QSL("PAUSED"));
                summaryLiveStatus->setPalette(pausedPalette);
            }
        });
    }

    { /// Frequency and sampling rate labels

        m_labels.summaryFrequency->setText(FMT_FIELD("0.0", " Hz"));
        m_labels.summaryFrequency->setAlignment(Qt::AlignRight);

        m_labels.summarySamplingRate->setText(FMT_FIELD("0.0", " samples/s"));
        m_labels.summarySamplingRate->setAlignment(Qt::AlignRight);

        m_labels.summaryLastSampleTime->setText("_");
        m_labels.summaryLastSampleTime->setAlignment(Qt::AlignRight);
    }
}

QVector<SidePanelItem> PhasorMonitor::sidePanelItems() const
{
    QVector<SidePanelItem> items;

    items << SidePanelItem{ m_ctrl.visibility.box, "Visibility" };
    items << SidePanelItem{ m_ctrl.phaseRef.box, "Phase Reference" };
    items << SidePanelItem{ m_ctrl.ampliScale.box, "Scale" };

    return items;
}

#undef QSL