#include "monitor_view.h"
#include "qpmu/common.h"
#include "util.h"

#include <iostream>
#include <qabstractseries.h>
#include <qlineseries.h>
#include <qnamespace.h>
#include <qradiobutton.h>

QPointF unitvector(qreal angle)
{
    return QPointF(std::cos(angle), std::sin(angle));
}

MonitorView::MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    using namespace qpmu;
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    m_updateNotifier = new TimeoutNotifier(updateTimer, UpdateIntervalMs);
    connect(m_updateNotifier, &TimeoutNotifier::timeout, this, &MonitorView::update);

    /// Layout
    auto outerHBox = new QHBoxLayout(this);
    setLayout(outerHBox);

    /// Data visualization area layout
    auto dataVBox = new QVBoxLayout(this);
    outerHBox->addLayout(dataVBox);

    /// ChartView
    auto chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    dataVBox->addWidget(chartView);

    /// Chart
    auto chart = new QChart();
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);
    chartView->setChart(chart);

    /// X-axis
    auto axisX = new QValueAxis(chart);
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setLabelsVisible(false);
    axisX->setRange(Scale * -0.05, (PolarGraphWidth + RectGraphWidth + Spacing) * Scale);
    axisX->setTickCount(2);
    axisX->setVisible(false);

    /// Y-axis
    auto axisY = new QValueAxis(chart);
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setLabelsVisible(false);
    axisY->setRange(Scale * -(0.05 + PolarGraphWidth / 2), Scale * +(0.05 + PolarGraphWidth / 2));
    axisY->setTickCount(2);
    axisY->setVisible(false);

    /// Fake axes
    auto makeFakeAxisSeries = [=](std::string type, qreal penWidth = 1) {
        QXYSeries *s = nullptr;
        if (type == "spline") {
            s = new QSplineSeries(chart);
        } else if (type == "line") {
            s = new QLineSeries(chart);
        } else if (type == "scatter") {
            s = new QScatterSeries(chart);
        }
        Q_ASSERT(s);
        auto color = QColor("lightGray");
        s->setPen(QPen(color, penWidth));
        if (type == "scatter") {
            s->setBrush(QBrush(color));
            s->setMarkerSize(0);
        }
        chart->addSeries(s);
        s->attachAxis(axisX);
        s->attachAxis(axisY);
        return s;
    };
    { /// Polar graph axes
        const auto xOffset = QPointF(PolarGraphWidth / 2, 0);
        for (int radiusPoint = 1; radiusPoint <= 4; ++radiusPoint) {
            const qreal radius = qreal(radiusPoint) / 4 * PolarGraphWidth / 2;
            auto circle = makeFakeAxisSeries("spline", 1 + bool(radiusPoint == 4) * 1.5);
            for (int i = 0; i <= (360 / 10); ++i) {
                const qreal theta = qreal(i) / (360.0 / 10) * (2 * M_PI);
                circle->append(Scale * (radius * unitvector(theta) + xOffset));
            }
        }
        for (int i = 0; i <= (360 / 30); ++i) {
            qreal theta = qreal(i) / (360.0 / 30) * (2 * M_PI);
            auto tick = makeFakeAxisSeries("line", 1);
            tick->append(Scale * xOffset);
            tick->append(Scale * (1.05 * unitvector(theta) + xOffset));
        }
    }
    { /// Rectangular graph axes
        auto ver = makeFakeAxisSeries("line", 2);
        ver->append(Scale * QPointF(Spacing + PolarGraphWidth, -(PolarGraphWidth / 2)));
        ver->append(Scale * QPointF(Spacing + PolarGraphWidth, +(PolarGraphWidth / 2)));
        auto hor = makeFakeAxisSeries("line", 2);
        hor->append(Scale * QPointF(Spacing + PolarGraphWidth, 0));
        hor->append(Scale * QPointF(Spacing + PolarGraphWidth + RectGraphWidth, 0));
    }

    /// Charts
    for (SizeType i = 0; i < NumChannels; ++i) {
        auto name = QString(Signals[i].name);
        auto color = QColor(Signals[i].colorHex);
        auto phasorPen = QPen(color, 3.0, Qt::SolidLine);
        auto waveformPen = QPen(color, 1.5, Qt::SolidLine);
        auto connectorPen = QPen(color.lighter(), 0.75, Qt::CustomDashLine);
        connectorPen.setDashPattern(QVector<qreal>() << (Scale * 2) << (Scale * 2));

        /// Phasor series
        auto phasorSeries = new QLineSeries(chart);
        phasorSeries->setName(name);
        phasorSeries->setPen(phasorPen);
        chart->addSeries(phasorSeries);
        phasorSeries->attachAxis(axisX);
        phasorSeries->attachAxis(axisY);

        /// Waveform series
        auto waveformSeries =
                signal_is_voltage(Signals[i]) ? new QSplineSeries(chart) : new QLineSeries(chart);
        waveformSeries->setName(name);
        waveformSeries->setPen(waveformPen);
        chart->addSeries(waveformSeries);
        waveformSeries->attachAxis(axisX);
        waveformSeries->attachAxis(axisY);

        /// Connector series
        auto connectorSeries = new QLineSeries(chart);
        connectorSeries->setName(name);
        connectorSeries->setPen(connectorPen);
        connectorSeries->setVisible(false);
        chart->addSeries(connectorSeries);
        connectorSeries->attachAxis(axisX);
        connectorSeries->attachAxis(axisY);

        m_plottedPhasors[i] = 0;
        m_phasorSeriesList[i] = phasorSeries;
        m_waveformSeriesList[i] = waveformSeries;
        m_connectorSeriesList[i] = connectorSeries;
        m_phasorPointsList[i] = QVector<QPointF>(5);
        m_waveformPointsList[i] = QVector<QPointF>(NumPointsPerCycle * NumCycles + 1);
        m_connectorPointsList[i] = QVector<QPointF>(2);
    }

    /// Tables
    auto tableLayout = new QGridLayout(this);
    dataVBox->addLayout(tableLayout);

    /// Table vertical headers (one for each phase)
    for (SizeType p = 0; p < NumPhases; ++p) {
        auto label = new QLabel(QString(SignalPhaseId[p]), this);
        label->setAlignment(Qt::AlignRight);
        if (p == 0) {
            label->setText(label->text() + QStringLiteral(" (Ref.)"));
        }
        tableLayout->addWidget(label, p + 1, 0);
    }

    /// Table horizontal headers (voltage, current, phase diff., power)
    for (int i = 0; i < NumTableColumns; ++i) {
        auto label = new QLabel(TableHHeaders[i], this);
        label->setAlignment(Qt::AlignCenter);
        if (i == 0) {
            label->setAlignment(Qt::AlignRight);
        }
        tableLayout->addWidget(label, 0, i);
    }

    for (SizeType p = 0; p < NumPhases; ++p) {
        const auto &[vIdx, iIdx] = SignalPhasePairs[p];
        auto vColorLabel = new QLabel(this);
        vColorLabel->setPixmap(circlePixmap(QColor(Signals[vIdx].colorHex), 10));
        auto vPhasorLabel = new QLabel("_", this);
        auto iColorLabel = new QLabel(this);
        iColorLabel->setPixmap(circlePixmap(QColor(Signals[iIdx].colorHex), 10));
        auto iPhasorLabel = new QLabel("_", this);
        auto phaseDiffLabel = new QLabel("_", this);
        auto powerLabel = new QLabel("_", this);

        vColorLabel->setAlignment(Qt::AlignRight);
        vPhasorLabel->setAlignment(Qt::AlignLeft);
        iColorLabel->setAlignment(Qt::AlignRight);
        iPhasorLabel->setAlignment(Qt::AlignLeft);
        phaseDiffLabel->setAlignment(Qt::AlignCenter);
        powerLabel->setAlignment(Qt::AlignCenter);

        m_phasorValueLabels[vIdx] = vPhasorLabel;
        m_phasorValueLabels[iIdx] = iPhasorLabel;
        m_phaseDiffLabels[p] = phaseDiffLabel;
        m_phasePowerLabels[p] = powerLabel;

        tableLayout->addWidget(vColorLabel, p + 1, 1);
        tableLayout->addWidget(vPhasorLabel, p + 1, 2);
        tableLayout->addWidget(iColorLabel, p + 1, 3);
        tableLayout->addWidget(iPhasorLabel, p + 1, 4);
        tableLayout->addWidget(phaseDiffLabel, p + 1, 5);
        tableLayout->addWidget(powerLabel, p + 1, 6);
    }

    /// Layout for constrols (side panel)
    auto sideVBox = new QVBoxLayout(this);
    sideVBox->setContentsMargins(QMargins(0, 20, 0, 20));
    outerHBox->addLayout(sideVBox);

    /// Enable/disable plots
    {
        auto groupBox = new QGroupBox("Hide or Show", this);
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);

        /// Connectors
        auto checkConnectors = new QCheckBox("Connector lines", this);
        connect(checkConnectors, &QCheckBox::toggled, [=](bool checked) {
            for (SizeType i = 0; i < NumChannels; ++i) {
                m_connectorSeriesList[i]->setVisible(checked && m_phasorSeriesList[i]->isVisible());
            }
        });
        checkConnectors->setChecked(true);

        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {
            /// Individual voltages or currents
            QList<QCheckBox *> checks;
            for (SizeType i = 0; i < NumChannels; ++i) {
                if (Signals[i].type == type) {
                    auto check = new QCheckBox(this);
                    connect(check, &QCheckBox::toggled, [=](bool checked) {
                        m_phasorSeriesList[i]->setVisible(checked);
                        m_waveformSeriesList[i]->setVisible(checked);
                        m_connectorSeriesList[i]->setVisible(checked
                                                             && checkConnectors->isChecked());
                    });
                    check->setChecked(true);
                    check->setIcon(circlePixmap(QColor(Signals[i].colorHex), 10));
                    check->setText(Signals[i].name);
                    groupGrid->addWidget(check, checks.size(), type);
                    checks.append(check);
                }
            }

            /// All voltages or all currents
            auto checkAll =
                    new QCheckBox((type == SignalType::Voltage ? QStringLiteral("Voltages")
                                                               : QStringLiteral("Currents")),
                                  this);
            connect(checkAll, &QCheckBox::toggled, [=](bool checked) {
                for (auto check : checks) {
                    check->setChecked(checked);
                }
            });
            checkAll->setChecked(true);
            groupGrid->addWidget(checkAll, checks.size(), type);

            /// If all checks are checked, then the "All" check should be checked
            for (auto check : checks) {
                connect(check, &QCheckBox::toggled, [=] {
                    int cnt = std::count_if(checks.begin(), checks.end(),
                                            [](QCheckBox *r) { return r->isChecked(); });
                    if (cnt == 0) {
                        checkAll->setChecked(false);
                    } else if (cnt == checks.size()) {
                        checkAll->setChecked(true);
                    }
                });
            }
        }

        groupGrid->addWidget(checkConnectors, groupGrid->rowCount(), 0, 1, 2);
    }

    /// Plot ranges
    {
        auto groupBox = new QGroupBox("Set Plot Range", this);
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);

        m_plotRangeIndex[0] = m_plotRangeIndex[1] = 0;

        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {
            auto combo = new QComboBox(this);
            for (int i = 0; i < (int)PlotRangeOptions[type].size(); ++i) {
                auto text = PlotRangeOptions[type][i] == 0
                        ? QStringLiteral("Auto")
                        : (QString::number(PlotRangeOptions[type][i])
                           + (type == SignalType::Voltage ? QStringLiteral(" V")
                                                          : QStringLiteral(" A")));
                combo->addItem(text);
            }
            connect(combo, &QComboBox::currentIndexChanged,
                    [=](int index) { m_plotRangeIndex[type] = index; });
            combo->setCurrentIndex(0);
            auto label = new QLabel((type == SignalType::Voltage ? QStringLiteral("Voltage")
                                                                 : QStringLiteral("Current")),
                                    this);
            groupGrid->addWidget(label, type, 0);
            groupGrid->addWidget(combo, type, 1);
        }
    }

    /// Frequency Simulation
    {
        auto groupBox = new QGroupBox("Simulate Frequency", this);
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);
        QList<QRadioButton *> radioButtons;
        m_simulationFrequencyIndex = 0;
        int i = 0;
        for (FloatType f : SimulationFrequencyOptions) {
            QString text = (f == 0) ? "Off" : QString::number(f) + " Hz";
            auto radio = new QRadioButton(text, this);
            connect(radio, &QRadioButton::toggled, [=](bool checked) {
                if (checked) {
                    m_simulationFrequencyIndex = i;
                }
            });
            if (i == 0) {
                radio->setChecked(true);
            }
            groupGrid->addWidget(radio, (i % 3), (i / 3));
            radioButtons.append(radio);
            ++i;
        }
    }
}

void MonitorView::update()
{
    using namespace qpmu;

    if (!isVisible())
        return;

    Estimations est;
    m_worker->getEstimations(est);

    std::array<FloatType, NumChannels> phaseDiffs;
    std::array<FloatType, NumChannels> amplitudes;

    FloatType phaseRef = std::arg(est.phasors[0]);
    for (SizeType i = 0; i < NumChannels; ++i) {
        phaseDiffs[i] = std::arg(est.phasors[i]) - phaseRef;
        phaseDiffs[i] = std::fmod(phaseDiffs[i] + 2 * M_PI, 2 * M_PI);
        amplitudes[i] = std::abs(est.phasors[i]);
    }

    std::array<FloatType, NumChannels> plotPhaseDiffs = phaseDiffs;
    std::array<FloatType, NumChannels> plotAmplitudes = amplitudes;

    FloatType maxAmpli[2] = { 0, 0 };
    for (SizeType i = 0; i < NumChannels; ++i) {
        maxAmpli[Signals[i].type] = std::max(maxAmpli[Signals[i].type], plotAmplitudes[i]);
    }
    for (SizeType i = 0; i < NumChannels; ++i) {
        if (m_plotRangeIndex[Signals[i].type] > 0) {
            plotAmplitudes[i] /= m_plotRangeIndex[Signals[i].type];
        } else {
            plotAmplitudes[i] /= maxAmpli[Signals[i].type];
        }
    }

    if (m_simulationFrequencyIndex > 0) {
        /// simulating; nudge the phasors according to the chosen frequency
        FloatType f = SimulationFrequencyOptions[m_simulationFrequencyIndex];
        FloatType delta = 2 * M_PI * f * UpdateIntervalMs / 1000.0;
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_plottedPhasors[i] *= std::polar(1.0, delta);
        }
    } else {
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_plottedPhasors[i] = std::polar(plotAmplitudes[i], plotPhaseDiffs[i]);
        }
    }

    for (SizeType i = 0; i < NumChannels; ++i) {
        auto phasor = m_plottedPhasors[i];
        auto phase = std::arg(phasor);
        auto ampli = std::abs(phasor);
        ampli *= 0.995;

        /// Phasor series path: origin > phasor > arrow-left-arm > phasor > arrow-right-arm
        /// ---
        /// Alpha = angle that the each of the two arrow arms makes with the phasor line at the
        /// origin
        qreal alpha;
        qreal alphaHypotenuse;
        {
            qreal alphaOpposite = 0.05;
            qreal alphaAdjacent = ampli - (2 * alphaOpposite);
            alpha = std::atan(alphaOpposite / alphaAdjacent);
            alphaHypotenuse =
                    std::sqrt(alphaOpposite * alphaOpposite + alphaAdjacent * alphaAdjacent);
        }
        m_phasorPointsList[i][0] = QPointF(0, 0);
        m_phasorPointsList[i][1] = ampli * unitvector(phase);
        m_phasorPointsList[i][2] = alphaHypotenuse * unitvector(phase + alpha);
        m_phasorPointsList[i][3] = m_phasorPointsList[i][1];
        m_phasorPointsList[i][4] = alphaHypotenuse * unitvector(phase - alpha);
        // if ampli > 1, which may be the case when the user picks an amplitude range for plotting,
        // clip the phasor to the unit circle and remove the arros
        if (ampli > 1 + 1e-9) {
            ampli = 1;
            m_phasorPointsList[i][1] = ampli * unitvector(phase);
            m_phasorPointsList[i][2] = m_phasorPointsList[i][0];
            m_phasorPointsList[i][3] = m_phasorPointsList[i][1];
            m_phasorPointsList[i][4] = m_phasorPointsList[i][0];
        }

        /// Waveform points are generated by simulating the sinusoid
        /// ---
        /// Voltage -> pure sine wave
        /// Current -> modified sine wave
        for (SizeType j = 0; j < (NumPointsPerCycle * NumCycles + 1); ++j) {
            FloatType t = j * (1.0 / NumPointsPerCycle);
            FloatType y = ampli * std::sin(-2 * M_PI * t + phase);
            FloatType x = j * (RectGraphWidth / NumCycles / NumPointsPerCycle);
            m_waveformPointsList[i][j] = QPointF(x, y);
        }
        if (signal_is_current(Signals[i])) {
            for (SizeType j = 2; j < (NumPointsPerCycle * NumCycles + 1); j += 2) {
                auto a = j - 2, b = j - 1, c = j;
                m_waveformPointsList[i][b].setX(m_waveformPointsList[i][c].x());
                m_waveformPointsList[i][b].setY(m_waveformPointsList[i][a].y());
            }
        }

        /// Translate the points to fit the designated areas
        for (SizeType j = 0; j < 5; ++j) {
            m_phasorPointsList[i][j] += QPointF(PolarGraphWidth / 2, 0);
        }
        for (SizeType j = 0; j < (NumPointsPerCycle * NumCycles + 1); ++j) {
            m_waveformPointsList[i][j] += QPointF(PolarGraphWidth + Spacing, 0);
        }

        /// Scale the points
        for (SizeType j = 0; j < 5; ++j) {
            m_phasorPointsList[i][j] *= Scale;
        }
        for (SizeType j = 0; j < (NumPointsPerCycle * NumCycles + 1); ++j) {
            m_waveformPointsList[i][j] *= Scale;
        }

        /// Connector points connect the phasor tip to the waveforms first point
        m_connectorPointsList[i][0] = m_phasorPointsList[i][1];
        m_connectorPointsList[i][1] = m_waveformPointsList[i][0];
    }

    for (SizeType i = 0; i < NumChannels; ++i) {
        m_phasorSeriesList[i]->replace(m_phasorPointsList[i]);
        m_waveformSeriesList[i]->replace(m_waveformPointsList[i]);
        m_connectorSeriesList[i]->replace(m_connectorPointsList[i]);
    }

    if (m_simulationFrequencyIndex > 0) {
        for (SizeType p = 0; p < NumPhases; ++p) {
            const auto &[vIdx, iIdx] = SignalPhasePairs[p];
            m_phasorValueLabels[vIdx]->setText("_");
            m_phasorValueLabels[iIdx]->setText("_");
            m_phaseDiffLabels[p]->setText("_");
            m_phasePowerLabels[p]->setText("_");
        }
    } else {
        for (SizeType p = 0; p < NumPhases; ++p) {
            const auto &[vIdx, iIdx] = SignalPhasePairs[p];
            const auto &vAmpli = amplitudes[vIdx];
            const auto &iAmpli = amplitudes[iIdx];
            const auto &vPhase = phaseDiffs[vIdx] * 180 / M_PI;
            const auto &iPhase = phaseDiffs[iIdx] * 180 / M_PI;
            const auto &phaseDiff = std::abs(std::fmod(vPhase - iPhase + 2 * 360, 360));
            const auto &power = est.power[p];

            m_phasorValueLabels[vIdx]->setText("<b>" + QString::number(vAmpli, 'f', 2) + "</b> "
                                               + SignalTypeUnit[SignalType::Voltage] + "  ∠  <b>"
                                               + QString::number(vPhase, 'f', 2) + "</b>°");
            m_phasorValueLabels[iIdx]->setText("<b>" + QString::number(iAmpli, 'f', 2) + "</b> "
                                               + SignalTypeUnit[SignalType::Current] + "  ∠  <b>"
                                               + QString::number(iPhase, 'f', 2) + "</b>°");
            m_phaseDiffLabels[p]->setText("<b>" + QString::number(phaseDiff, 'f', 2) + "</b>°");
            m_phasePowerLabels[p]->setText("<b>" + QString::number(power, 'f', 2) + "</b> W");
        }
    }
}
