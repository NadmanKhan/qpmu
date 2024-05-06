#include "monitor_view.h"
#include "util.h"

MonitorView::MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    using namespace qpmu;
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    /// Update notifier
    m_updateNotifier = new TimeoutNotifier(updateTimer, UpdateIntervalMs);
    connect(m_updateNotifier, &TimeoutNotifier::timeout, this, &MonitorView::update);

    /// Main outer layout
    auto outerHBox = new QHBoxLayout(this);

    /// Data visualization area layout
    auto dataVBox = new QVBoxLayout();
    outerHBox->addLayout(dataVBox);

    /// ChartView
    auto chartView = new QChartView();
    dataVBox->addWidget(chartView, 2);
    chartView->setRenderHint(QPainter::Antialiasing, true);

    /// Chart
    auto chart = new EquallyScaledAxesChart();
    chartView->setChart(chart);
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);

    /// Axes
    auto axisX = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setLabelsVisible(false);
    axisX->setRange(-0.05, (PolarGraphWidth + RectGraphWidth + Spacing));
    axisX->setTickCount(2);
    axisX->setVisible(false);
    auto axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setLabelsVisible(false);
    axisY->setRange(-(0.05 + PolarGraphWidth / 2), +(0.05 + PolarGraphWidth / 2));
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
            auto circle = makeFakeAxisSeries("spline", 1 + bool(radiusPoint == 4) * 1);
            for (int i = 0; i <= (360 / 10); ++i) {
                const qreal theta = qreal(i) / (360.0 / 10) * (2 * M_PI);
                circle->append((radius * unitvector(theta) + xOffset));
            }
        }
        for (int i = 0; i <= (360 / 30); ++i) {
            qreal theta = qreal(i) / (360.0 / 30) * (2 * M_PI);
            auto tick = makeFakeAxisSeries("line", 1);
            tick->append(xOffset);
            tick->append((1.05 * unitvector(theta) + xOffset));
        }
    }
    { /// Rectangular graph axes
        auto ver = makeFakeAxisSeries("line", 2);
        ver->append(QPointF(Spacing + PolarGraphWidth, -(PolarGraphWidth / 2)));
        ver->append(QPointF(Spacing + PolarGraphWidth, +(PolarGraphWidth / 2)));
        auto hor = makeFakeAxisSeries("line", 2);
        hor->append(QPointF(Spacing + PolarGraphWidth, 0));
        hor->append(QPointF(Spacing + PolarGraphWidth + RectGraphWidth, 0));
    }

    /// Data
    for (SizeType i = 0; i < NumChannels; ++i) {
        auto name = QString(Signals[i].name);
        auto color = QColor(Signals[i].colorHex);
        auto phasorPen = QPen(color, 3.0, Qt::SolidLine);
        auto waveformPen = QPen(color, 1.5, Qt::SolidLine);
        auto connectorPen = QPen(color, 1.0, Qt::CustomDashLine);
        connectorPen.setDashPattern(QVector<qreal>() << 15 << 15);

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

    /// Data table
    auto table = new QTableWidget();
    dataVBox->addWidget(table, 1);
    table->setColumnCount(NumTableColumns);
    table->setRowCount(NumPhases);
    table->setContentsMargins(QMargins(10, 0, 10, 0));
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);

    chartView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    table->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    {
        auto font = table->font();
        // font.setPointSize(font.pointSize() * 1.25);
        font.setFamily(QStringLiteral("monospace"));
        table->setFont(font);
    }

    // Clear cell selections because they are unwanted
    connect(table, &QTableWidget::currentCellChanged, [=] { table->setCurrentCell(-1, -1); });

    auto createTableHeader = [&] {
        auto item = new QTableWidgetItem();
        item->setFlags(Qt::NoItemFlags);
        item->setFont(QFont(QStringLiteral("monospace")));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    /// Table vertical headers (one for each phase)
    for (SizeType p = 0; p < NumPhases; ++p) {
        auto label = new QLabel(QString(SignalPhaseId[p]));
        label->setAlignment(Qt::AlignRight);
        auto item = createTableHeader();
        item->setText(label->text());
        table->setVerticalHeaderItem(p, item);
    }

    /// Table horizontal headers (voltage, current, phase diff., power)
    for (int i = 0; i < NumTableColumns; ++i) {
        auto label = new QLabel(TableHHeaders[i]);
        label->setAlignment(Qt::AlignRight);
        auto item = createTableHeader();
        item->setText(label->text());
        table->setHorizontalHeaderItem(i, item);
    }

    const auto blankText = QStringLiteral("_");

    for (SizeType p = 0; p < NumPhases; ++p) {
        const auto &[vIdx, iIdx] = SignalPhasePairs[p];

        auto vPhasorLabel = new QLabel(blankText);
        auto iPhasorLabel = new QLabel(blankText);
        auto phaseDiffLabel = new QLabel(blankText);
        auto powerLabel = new QLabel(blankText);

        m_phasorLabels[vIdx] = vPhasorLabel;
        m_phasorLabels[iIdx] = iPhasorLabel;
        m_phaseDiffLabels[p] = phaseDiffLabel;
        m_phasePowerLabels[p] = powerLabel;

        auto vColorLabel = new QLabel();
        vColorLabel->setPixmap(circlePixmap(QColor(Signals[vIdx].colorHex), 10));
        vColorLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        vPhasorLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        auto vWidget = new QWidget();
        auto vHBox = new QHBoxLayout(vWidget);
        vHBox->setContentsMargins(QMargins(0, 0, 0, 0));
        vHBox->setAlignment(Qt::AlignCenter);
        vHBox->setSpacing(0);
        vHBox->addWidget(vColorLabel);
        vHBox->addWidget(vPhasorLabel);

        auto iColorLabel = new QLabel();
        iColorLabel->setPixmap(circlePixmap(QColor(Signals[iIdx].colorHex), 10));
        iColorLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        iPhasorLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        auto iWidget = new QWidget();
        auto iHBox = new QHBoxLayout(iWidget);
        iHBox->setContentsMargins(QMargins(0, 0, 0, 0));
        iHBox->setAlignment(Qt::AlignCenter);
        iHBox->setSpacing(0);
        iHBox->addWidget(iColorLabel);
        iHBox->addWidget(iPhasorLabel);

        phaseDiffLabel->setAlignment(Qt::AlignCenter);

        powerLabel->setAlignment(Qt::AlignCenter);

        for (auto &label : { vPhasorLabel, iPhasorLabel, phaseDiffLabel, powerLabel }) {
            auto font = label->font();
            font.setPointSize(font.pointSize() * 1.25);
            label->setFont(font);
            label->setContentsMargins(QMargins(5, 0, 5, 0));
        }

        table->setCellWidget(p, 0, vWidget);
        table->setCellWidget(p, 1, iWidget);
        table->setCellWidget(p, 2, phaseDiffLabel);
        table->setCellWidget(p, 3, powerLabel);
    }

    /// Layout for controls (side panel)
    auto sideVBox = new QVBoxLayout();
    outerHBox->addLayout(sideVBox);
    sideVBox->setContentsMargins(QMargins(0, 20, 0, 20));

    /// Toggle visibility
    {
        auto groupBox = new QGroupBox("Toggle Visibility");
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);

        /// Connectors
        auto checkConnectors = new QCheckBox("Connector lines");
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
                    auto check = new QCheckBox();
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
                                                               : QStringLiteral("Currents")));
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

    /// Set plot range
    {
        auto groupBox = new QGroupBox("Set Plot Range");
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);

        m_plotRangeIndex[0] = m_plotRangeIndex[1] = 0;

        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {
            auto combo = new QComboBox();
            for (int i = 0; i < (int)PlotRangeOptions[type].size(); ++i) {
                auto text = PlotRangeOptions[type][i] == 0
                        ? QStringLiteral("Auto")
                        : (QString::number(PlotRangeOptions[type][i])
                           + (type == SignalType::Voltage ? QStringLiteral(" V")
                                                          : QStringLiteral(" A")));
                combo->addItem(text);
            }
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [=](int index) { m_plotRangeIndex[type] = index; });
            combo->setCurrentIndex(0);
            auto label = new QLabel((type == SignalType::Voltage ? QStringLiteral("Voltage")
                                                                 : QStringLiteral("Current")));
            groupGrid->addWidget(label, type, 0);
            groupGrid->addWidget(combo, type, 1);
        }
    }

    /// Simulate frequency
    {
        auto groupBox = new QGroupBox("Simulate Frequency");
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);
        QList<QRadioButton *> radioButtons;
        m_simulationFrequencyIndex = 0;
        int i = 0;
        for (FloatType f : SimulationFrequencyOptions) {
            auto radio = new QRadioButton((f == 0) ? "Off" : QString::number(f) + " Hz");
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
        if (phaseDiffs[i] < M_PI) {
            phaseDiffs[i] += 2 * M_PI;
        }
        if (phaseDiffs[i] > M_PI) {
            phaseDiffs[i] -= 2 * M_PI;
        }
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
            plotAmplitudes[i] /=
                    PlotRangeOptions[Signals[i].type][m_plotRangeIndex[Signals[i].type]];
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
            qreal alphaOpposite = 0.03;
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
            m_phasorLabels[vIdx]->setText("_");
            m_phasorLabels[iIdx]->setText("_");
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
            auto diff = std::abs(vPhase - iPhase);
            const auto &phaseDiff = std::min(diff, 360 - diff);
            const auto &power = est.power[p];

            auto vPhasorText = QStringLiteral("<pre><strong>%1</strong><small> V ∠ "
                                              "</small><strong>%2</strong><small> °</small></pre>")
                                       .arg(vAmpli, 6, 'f', 1, ' ')
                                       .arg(vPhase, 6, 'f', 1, ' ');
            auto iPhasorText = QStringLiteral("<pre><strong>%1</strong><small> A ∠ "
                                              "</small><strong>%2</strong><small> °</small></pre>")
                                       .arg(iAmpli, 6, 'f', 1, ' ')
                                       .arg(iPhase, 6, 'f', 1, ' ');
            auto phaseDiffText = QStringLiteral("<pre><strong>%1</strong><small> °</small></pre>")
                                         .arg(phaseDiff, 6, 'f', 1, ' ');
            auto powerText = QStringLiteral("<pre><strong>%1</strong><small> W</small></pre>")
                                     .arg(power, 8, 'f', 1, ' ');

            m_phasorLabels[vIdx]->setText(vPhasorText);
            m_phasorLabels[iIdx]->setText(iPhasorText);
            m_phaseDiffLabels[p]->setText(phaseDiffText);
            m_phasePowerLabels[p]->setText(powerText);
        }
    }
}
