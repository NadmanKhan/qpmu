#include "monitor_view.h"
#include "qpmu/common.h"
#include "util.h"

#include <cmath>
#include <qabstractseries.h>
#include <qcheckbox.h>
#include <qdebug.h>
#include <qlineseries.h>
#include <qnamespace.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qstringliteral.h>

MonitorView::MonitorView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    using namespace qpmu;
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    /// Update notifiers
    m_simulationUpdateNotifier = new TimeoutNotifier(updateTimer, UpdateIntervalMs);
    connect(m_simulationUpdateNotifier, &TimeoutNotifier::timeout, this,
            &MonitorView::noForceUpdate);
    m_updateNotifier = new TimeoutNotifier(updateTimer, UpdateIntervalMs * 5);
    connect(m_updateNotifier, &TimeoutNotifier::timeout, this, &MonitorView::noForceUpdate);

    /// Main outer layout
    auto outerHBox = new QHBoxLayout(this);

    /// Data visualization area layout
    auto dataVBox = new QVBoxLayout();
    outerHBox->addLayout(dataVBox, 1);

    /// ChartView
    auto chartView = new QChartView();
    dataVBox->addWidget(chartView, 1);
    chartView->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setContentsMargins(QMargins(0, 0, 0, 0));

    /// Status and frqequency labels
    auto labelsHBox = new QHBoxLayout();
    dataVBox->addLayout(labelsHBox);
    m_statusLabel = new QLabel();
    m_frequencyLabel = new QLabel();
    labelsHBox->addWidget(m_statusLabel, 0);
    labelsHBox->addWidget(m_frequencyLabel, 1);
    m_statusLabel->setAlignment(Qt::AlignLeft);
    m_frequencyLabel->setAlignment(Qt::AlignRight);
    for (auto label : { m_statusLabel, m_frequencyLabel }) {
        label->setAutoFillBackground(true);
        label->setBackgroundRole(QPalette::Highlight);
        label->setForegroundRole(QPalette::BrightText);
        label->setContentsMargins(QMargins(10, 5, 10, 5));
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        {
            auto font = label->font();
            font.setPointSize(font.pointSize() * 1.5);
            label->setFont(font);
        }
    }
    {
        auto palette = m_statusLabel->palette();
        palette.setColor(QPalette::Highlight, QStringLiteral("#22bb45"));
        m_statusLabel->setPalette(palette);
    }
    {
        auto font = m_frequencyLabel->font();
        font.setFamily(QStringLiteral("monospace"));
        m_frequencyLabel->setFont(font);
    }

    /// Chart
    auto chart = new EquallyScaledAxesChart();
    chartView->setChart(chart);
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setBackgroundRoundness(0);
    chart->setContentsMargins(0, 0, 0, 0);

    /// Axes
    auto axisX = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setLabelsVisible(false);
    axisX->setRange(-0.05, 0.05 + (PolarGraphWidth + RectGraphWidth + Spacing));
    axisX->setTickCount(2);
    axisX->setVisible(false);
    auto axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setLabelsVisible(false);
    axisY->setRange(-(0.05 + PolarGraphWidth / 2), +(0.05 + PolarGraphWidth / 2));
    axisY->setTickCount(2);
    axisY->setVisible(false);

    /// Fake axes
    auto makeFakeAxisSeries = [=](QAbstractSeries::SeriesType type, qreal penWidth, QString name) {
        QLineSeries *s = nullptr;
        if (type == QAbstractSeries::SeriesTypeSpline) {
            s = new QSplineSeries(chart);
        } else if (type == QAbstractSeries::SeriesTypeLine) {
            s = new QLineSeries(chart);
        }
        Q_ASSERT(s);
        auto color = QColor("lightGray");
        s->setPen(QPen(color, penWidth));
        s->setName(name);
        chart->addSeries(s);
        s->attachAxis(axisX);
        s->attachAxis(axisY);
        return s;
    };
    { /// Polar graph axes
        constexpr qreal xoff[2] = { PolarGraphWidth / 2,
                                    (PolarGraphWidth + Spacing + RectGraphWidth) / 2 };
        for (bool b : { 0, 1 }) {
            auto &seriesList = m_phasorFakeAxesSeriesList[b];
            for (int radiusPercent : { 100, 89, 67 }) {
                const qreal radius = qreal(radiusPercent) / 100 * PolarGraphWidth / 2;
                auto circle = makeFakeAxisSeries(QAbstractSeries::SeriesTypeSpline, 1.5,
                                                 QStringLiteral("Angular Axis"));
                for (int i = 0; i <= (360 / 10); ++i) {
                    const qreal theta = qreal(i) / (360.0 / 10) * (2 * M_PI);
                    circle->append(radius * unitvector(theta));
                }
                seriesList.append(circle);
            }
            for (int i = 0; i <= (360 / 30); ++i) {
                qreal theta = qreal(i) / (360.0 / 30) * (2 * M_PI);
                auto angleTick =
                        makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 1,
                                           QStringLiteral("Angular Axis Tick at %1°").arg(i * 30));
                angleTick->append(0, 0);
                angleTick->append(1.05 * unitvector(theta));
                seriesList.append(angleTick);
            }
            for (auto series : seriesList) {
                for (int j = 0; j < (int)series->count(); ++j) {
                    series->replace(j, series->at(j) + QPointF(xoff[b], 0));
                }
            }
        }
    }
    { /// Rectangular graph axes
        constexpr qreal xoff[2] = { PolarGraphWidth + Spacing,
                                    ((PolarGraphWidth + Spacing + RectGraphWidth) / 2)
                                            - (RectGraphWidth / 2) };
        for (bool b : { 0, 1 }) {
            auto &seriesList = m_waveformFakeAxesSeriesList[b];
            auto ver = makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 2,
                                          QStringLiteral("Voltage or Current"));
            ver->append(0, -(PolarGraphWidth / 2));
            ver->append(0, +(PolarGraphWidth / 2));
            auto hor =
                    makeFakeAxisSeries(QAbstractSeries::SeriesTypeLine, 2, QStringLiteral("Time"));
            hor->append(0, 0);
            hor->append(RectGraphWidth, 0);
            seriesList.append(ver);
            seriesList.append(hor);
            for (auto series : m_waveformFakeAxesSeriesList[b]) {
                for (int j = 0; j < (int)series->count(); ++j) {
                    series->replace(j, series->at(j) + QPointF(xoff[b], 0));
                }
            }
        }
    }

    /// Data
    for (USize i = 0; i < NumChannels; ++i) {
        auto name = QString(Signals[i].name);
        auto color = QColor(Signals[i].colorHex);
        auto phasorPen = QPen(color, 3, Qt::SolidLine);
        auto waveformPen = QPen(color, 2, Qt::SolidLine);
        auto connectorPen = QPen(color, 1, Qt::DotLine);

        /// Phasor series
        auto phasorSeries = new QLineSeries(chart);
        phasorSeries->setName(name);
        phasorSeries->setPen(phasorPen);
        chart->addSeries(phasorSeries);
        phasorSeries->attachAxis(axisX);
        phasorSeries->attachAxis(axisY);

        /// Waveform series
        auto waveformSeries = new QSplineSeries(chart);
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

        m_phasorSeriesList[i] = phasorSeries;
        m_waveformSeriesList[i] = waveformSeries;
        m_connectorSeriesList[i] = connectorSeries;
        m_phasorPointsList[i] = QVector<QPointF>(5);
        m_waveformPointsList[i] = QVector<QPointF>(NumPointsPerCycle * NumCycles + 1);
        m_connectorPointsList[i] = QVector<QPointF>(2);
    }

    /// Data table
    auto table = new QTableWidget();
    dataVBox->addWidget(table, 0);
    table->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    table->setColumnCount(TableHHeaders.size());
    table->setRowCount(NumPhases);
    table->setContentsMargins(QMargins(10, 0, 10, 0));
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);

    {
        auto font = table->font();
        font.setFamily(QStringLiteral("monospace"));
        table->setFont(font);
    }

    table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
    {
        QStringList vHeaderLabels;
        for (USize p = 0; p < NumPhases; ++p) {
            vHeaderLabels << (" " + QString(SignalPhaseId[p]) + " ");
        }
        table->setVerticalHeaderLabels(vHeaderLabels);
    }

    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    {
        QStringList hHeaderLabels;
        for (int i = 0; i < (int)TableHHeaders.size(); ++i) {
            hHeaderLabels << TableHHeaders[i];
        }
        table->setHorizontalHeaderLabels(hHeaderLabels);
    }

    const auto blankText = QStringLiteral("_");

    for (USize p = 0; p < NumPhases; ++p) {
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
        auto vWidget = new QWidget();
        auto vHBox = new QHBoxLayout(vWidget);
        vHBox->addWidget(vColorLabel, 0);
        vHBox->addWidget(vPhasorLabel, 1);
        vHBox->setSpacing(0);
        vHBox->setAlignment(Qt::AlignCenter);
        vColorLabel->setPixmap(rectPixmap(QColor(Signals[vIdx].colorHex), 8, 20));
        vColorLabel->setContentsMargins(QMargins(0, 0, 0, 0));
        vHBox->setContentsMargins(QMargins(0, 0, 0, 0));

        auto iColorLabel = new QLabel();
        auto iWidget = new QWidget();
        auto iHBox = new QHBoxLayout(iWidget);
        iHBox->addWidget(iColorLabel, 0);
        iHBox->addWidget(iPhasorLabel, 1);
        iHBox->setSpacing(0);
        iHBox->setAlignment(Qt::AlignCenter);
        iColorLabel->setPixmap(rectPixmap(QColor(Signals[iIdx].colorHex), 8, 20));
        iColorLabel->setContentsMargins(QMargins(0, 0, 0, 0));
        iHBox->setContentsMargins(QMargins(0, 0, 0, 0));

        for (auto label : { vPhasorLabel, iPhasorLabel, phaseDiffLabel, powerLabel }) {
            auto font = label->font();
            label->setFont(font);
            label->setAlignment(Qt::AlignCenter);
            label->setContentsMargins(QMargins(0, 0, 0, 0));
        }

        table->setCellWidget(p, 0, vWidget);
        table->setCellWidget(p, 1, iWidget);
        table->setCellWidget(p, 2, phaseDiffLabel);
        table->setCellWidget(p, 3, powerLabel);
    }

    /// Layout for controls (side panel)
    auto verticalLine = new QFrame();
    verticalLine->setFrameShape(QFrame::VLine);
    verticalLine->setFrameShadow(QFrame::Sunken);
    outerHBox->addWidget(verticalLine);
    auto sideVBox = new QVBoxLayout();
    outerHBox->addLayout(sideVBox, 0);
    sideVBox->setContentsMargins(QMargins(0, 0, 0, 0));

    /// Pause/play
    m_pausePlayButton = new QPushButton(this);
    sideVBox->addWidget(m_pausePlayButton);
    {
        auto font = m_pausePlayButton->font();
        font.setPointSize(font.pointSize() * 1.5);
        m_pausePlayButton->setFont(font);
    }
    m_pausePlayButton->setAutoFillBackground(true);
    m_pausePlayButton->setBackgroundRole(QPalette::Highlight);
    m_pausePlayButton->setIcon(QIcon(QStringLiteral(":/pause.png")));
    m_pausePlayButton->setText(QStringLiteral("Pause"));
    connect(m_pausePlayButton, &QPushButton::clicked, [=] {
        m_playing ^= 1;
        if (m_playing) {
            m_pausePlayButton->setIcon(QIcon(QStringLiteral(":/pause.png")));
            m_pausePlayButton->setText(QStringLiteral("Pause"));
            m_statusLabel->setText(QStringLiteral("<strong>")
                                   + (isSimulating() ? QStringLiteral("SIMULATING ROTATION")
                                                     : QStringLiteral("LIVE"))
                                   + QStringLiteral("</strong>"));
            auto palette = m_statusLabel->palette();
            palette.setColor(
                    QPalette::Highlight,
                    (isSimulating() ? QStringLiteral("#808080") : QStringLiteral("#22bb45")));
            m_statusLabel->setPalette(palette);
        } else {
            m_pausePlayButton->setIcon(QIcon(QStringLiteral(":/play.png")));
            m_pausePlayButton->setText(QStringLiteral("Play"));
            m_statusLabel->setText(
                    QStringLiteral("<strong>PAUSED ")
                    + (isSimulating() ? QStringLiteral("SIMULATION") : QStringLiteral("LIVE"))
                    + QStringLiteral("</strong>"));
            auto palette = m_statusLabel->palette();
            palette.setColor(QPalette::Highlight, QStringLiteral("#dc143c"));
            m_statusLabel->setPalette(palette);
        }
        forceUpdate();
    });

    { /// Toggle visibility
        m_phasorCheckBox = new QCheckBox();
        m_waveformCheckBox = new QCheckBox();
        m_connectorCheckBox = new QCheckBox();

        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {
            /// Individual signals of SignalType == type
            for (USize i = 0; i < NumChannels; ++i) {
                if (Signals[i].type == type) {
                    auto check = new QCheckBox();
                    check->setIcon(circlePixmap(QColor(Signals[i].colorHex), 10));
                    check->setText(Signals[i].name);
                    m_signalCheckBoxList[type].append(check);
                    check->setChecked(true);
                }
            }

            /// All signals of SignalType == type
            auto checkAll = new QCheckBox();
            if (type == SignalType::Voltage) {
                checkAll->setText(QStringLiteral("Voltages"));
            } else {
                checkAll->setText(QStringLiteral("Currents"));
            }
            for (auto check : m_signalCheckBoxList[type]) {
                connect(check, QOverload<bool>::of(&QCheckBox::toggled), [=] {
                    int cnt = 0;
                    int max = 0;
                    for (auto cb : m_signalCheckBoxList[type]) {
                        max += (cb != checkAll);
                        cnt += (cb != checkAll && cb->isChecked());
                    }
                    if (cnt == 0) {
                        for (auto c : m_signalCheckBoxList[type]) {
                            c->setChecked(false);
                        }
                    }
                    if (cnt == max) {
                        for (auto c : m_signalCheckBoxList[type]) {
                            c->setChecked(true);
                        }
                    }
                });
            }
            connect(checkAll, QOverload<bool>::of(&QCheckBox::toggled), [=](bool checked) {
                for (auto c : m_signalCheckBoxList[type]) {
                    c->setChecked(checked);
                }
            });
            m_signalCheckBoxList[type].append(checkAll);
            checkAll->setChecked(true);
        }

        /// Phasors
        m_phasorCheckBox->setText(QStringLiteral("Phasors"));
        m_phasorCheckBox->setIcon(QIcon(QStringLiteral(":/vector.png")));
        m_phasorCheckBox->setChecked(true);

        /// Waveforms
        m_waveformCheckBox->setText(QStringLiteral("Waveforms"));
        m_waveformCheckBox->setIcon(QIcon(QStringLiteral(":/waves.png")));
        m_waveformCheckBox->setChecked(true);

        /// Connectors
        m_connectorCheckBox->setText(QStringLiteral("Connector lines"));
        m_connectorCheckBox->setIcon(QIcon(QStringLiteral(":/dashed-line.png")));
        m_connectorCheckBox->setChecked(true);

        /// Horizontal line
        auto horizontalLine = new QFrame();
        horizontalLine->setFrameShape(QFrame::HLine);
        horizontalLine->setFrameShadow(QFrame::Sunken);

        auto vizGroupBox = new QGroupBox(QStringLiteral("Toggle Visibility"));
        sideVBox->addWidget(vizGroupBox);

        auto vizGroupGrid = new QGridLayout(vizGroupBox);
        vizGroupGrid->addWidget(m_phasorCheckBox, 0, 0, 1, 2);
        vizGroupGrid->addWidget(m_waveformCheckBox, 1, 0, 1, 2);
        vizGroupGrid->addWidget(m_connectorCheckBox, 2, 0, 1, 2);
        vizGroupGrid->addWidget(horizontalLine, 3, 0, 1, 2);
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < (int)m_signalCheckBoxList[i].size(); ++j) {
                vizGroupGrid->addWidget(m_signalCheckBoxList[i][j], 4 + j, i);
            }
        }

        connect(m_phasorCheckBox, QOverload<bool>::of(&QCheckBox::toggled), this,
                &MonitorView::forceUpdate);
        connect(m_waveformCheckBox, QOverload<bool>::of(&QCheckBox::toggled), this,
                &MonitorView::forceUpdate);
        connect(m_connectorCheckBox, QOverload<bool>::of(&QCheckBox::toggled), this,
                &MonitorView::forceUpdate);
        for (int i = 0; i < 2; ++i) {
            for (auto checkBox : m_signalCheckBoxList[i]) {
                connect(checkBox, &QCheckBox::toggled, this, &MonitorView::forceUpdate);
            }
        }
    }

    { /// Plot max amplitude options
        auto ampliGroupBox = new QGroupBox(QStringLiteral("Set Upper Bound"));
        sideVBox->addWidget(ampliGroupBox);
        auto ampliGroupGrid = new QGridLayout(ampliGroupBox);

        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {
            auto combo = m_maxPlotAmplitude[type] = new QComboBox();
            for (int i = 0; i < (int)PlotAmpliOptions[type].size(); ++i) {
                auto text = PlotAmpliOptions[type][i] == 0
                        ? QStringLiteral("Auto")
                        : (QString::number(PlotAmpliOptions[type][i])
                           + (type == SignalType::Voltage ? QStringLiteral(" V")
                                                          : QStringLiteral(" A")));
                combo->addItem(text);
            }
            combo->setCurrentIndex(0);
            auto label = new QLabel((type == SignalType::Voltage ? QStringLiteral("Voltage")
                                                                 : QStringLiteral("Current")));
            ampliGroupGrid->addWidget(label, type, 0);
            ampliGroupGrid->addWidget(combo, type, 1);
        }
    }

    /// Simulate frequency
    {
        auto groupBox = new QGroupBox(QStringLiteral("Simulate Rotation"));
        sideVBox->addWidget(groupBox);
        auto groupGrid = new QGridLayout(groupBox);
        QList<QRadioButton *> radioButtons;

        for (int i = 0; i < (int)SimulationFrequencyOptions.size(); ++i) {
            auto f = SimulationFrequencyOptions[i];
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
        }
    }

    update(true);
}

void MonitorView::noForceUpdate()
{
    update(false);
}

void MonitorView::forceUpdate()
{
    update(true);
}

bool MonitorView::isSimulating() const
{
    return m_simulationFrequencyIndex > 0;
}

void MonitorView::update(bool force)
{
    using namespace qpmu;

    if (!force && !isVisible()) {
        return;
    }

    if (!m_playing) {
        return;
    }

    Estimation est; // Filled only if not simulating

    if (isSimulating()) {
        /// simulating; nudge the phasors according to the chosen frequency
        FloatType f = SimulationFrequencyOptions[m_simulationFrequencyIndex];
        FloatType delta = f * (2 * M_PI * UpdateIntervalMs / 1000.0);
        for (USize i = 0; i < NumChannels; ++i) {
            m_plotPhaseDiffs[i] = std::fmod(m_plotPhaseDiffs[i] + delta, 2 * M_PI);
            if (m_plotPhaseDiffs[i] < M_PI) {
                m_plotPhaseDiffs[i] += 2 * M_PI;
            }
            if (m_plotPhaseDiffs[i] > M_PI) {
                m_plotPhaseDiffs[i] -= 2 * M_PI;
            }
        }
    } else {
        if (!force && !m_updateNotifier->isTimeout()) {
            return;
        }

        m_worker->getEstimations(est);

        FloatType phaseRef = est.phasor_ang[0];
        for (USize i = 0; i < NumChannels; ++i) {
            m_plotAmplitudes[i] = est.phasor_mag[i];
            m_plotPhaseDiffs[i] = std::fmod(est.phasor_ang[i] - phaseRef + 2 * M_PI, 2 * M_PI);
            if (m_plotPhaseDiffs[i] < M_PI) {
                m_plotPhaseDiffs[i] += 2 * M_PI;
            }
            if (m_plotPhaseDiffs[i] > M_PI) {
                m_plotPhaseDiffs[i] -= 2 * M_PI;
            }
        }
    }

    FloatType plotMaxAmpli[2] = { 0, 0 };
    for (USize i = 0; i < NumChannels; ++i) {
        auto selected = m_maxPlotAmplitude[Signals[i].type]->currentIndex();
        if (selected > 0) {
            plotMaxAmpli[Signals[i].type] = PlotAmpliOptions[Signals[i].type][selected];
        } else {
            plotMaxAmpli[Signals[i].type] =
                    std::max(plotMaxAmpli[Signals[i].type], m_plotAmplitudes[i]);
        }
    }

    /// Update points
    for (USize i = 0; i < NumChannels; ++i) {
        auto ampli = m_plotAmplitudes[i] / plotMaxAmpli[Signals[i].type];
        auto phase = m_plotPhaseDiffs[i];
        bool ampliExceedsPlot = ampli > 1 + 1e-9;

        /// Phasor series path: origin > phasor > arrow-left-arm > phasor > arrow-right-arm
        /// ---
        /// Alpha = angle that the each of the two arrow arms makes with the phasor line at the
        /// origin

        if (ampliExceedsPlot) {
            m_phasorPointsList[i][0] = QPointF(0, 0);
            m_phasorPointsList[i][1] = unitvector(phase);
            m_phasorPointsList[i][2] = m_phasorPointsList[i][0];
            m_phasorPointsList[i][3] = m_phasorPointsList[i][1];
            m_phasorPointsList[i][4] = m_phasorPointsList[i][0];
        } else {
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
        }

        /// Waveform points are generated by simulating the sinusoid
        for (USize j = 0; j < (NumPointsPerCycle * NumCycles + 1); ++j) {
            FloatType t = j * (1.0 / NumPointsPerCycle);
            FloatType y = ampli * std::sin(-2 * M_PI * t + phase);
            FloatType x = j * (RectGraphWidth / (NumCycles * NumPointsPerCycle));
            m_waveformPointsList[i][j] = QPointF(x, y);
        }

        /// Translate the points to fit the designated areas
        qreal offsetPhasors = PolarGraphWidth / 2;
        qreal offsetWaveforms = PolarGraphWidth + Spacing;

        if (!m_phasorCheckBox->isChecked()) {
            offsetWaveforms =
                    ((PolarGraphWidth + Spacing + RectGraphWidth) / 2) - (RectGraphWidth / 2);
        }
        if (!m_waveformCheckBox->isChecked()) {
            offsetPhasors = ((PolarGraphWidth + Spacing + RectGraphWidth) / 2);
        }

        for (auto &point : m_phasorPointsList[i]) {
            point.setX(point.x() + offsetPhasors);
        }

        for (auto &point : m_waveformPointsList[i]) {
            point.setX(point.x() + offsetWaveforms);
        }

        /// Connector points connect the phasor tip to the waveforms first point
        if (ampliExceedsPlot) {
            m_connectorPointsList[i][0] = m_connectorPointsList[i][1] = m_phasorPointsList[i][1];
        } else {
            m_connectorPointsList[i][0] = m_phasorPointsList[i][1];
            m_connectorPointsList[i][1] = m_waveformPointsList[i][0];
        }
    }

    /// Set visibility
    for (USize i = 0; i < NumChannels; ++i) {
        bool checked = m_signalCheckBoxList[Signals[i].type][i % 3]->isChecked();
        m_phasorSeriesList[i]->setVisible(checked && m_phasorCheckBox->isChecked());
        m_waveformSeriesList[i]->setVisible(checked && m_waveformCheckBox->isChecked());
        m_connectorSeriesList[i]->setVisible(checked && m_connectorCheckBox->isChecked()
                                             && m_phasorCheckBox->isChecked()
                                             && m_waveformCheckBox->isChecked());
    }

    if (m_phasorCheckBox->isChecked()) {
        bool which = !m_waveformCheckBox->isChecked();
        for (auto &series : m_phasorFakeAxesSeriesList[which]) {
            series->setVisible(true);
        }
        for (auto &series : m_phasorFakeAxesSeriesList[which ^ 1]) {
            series->setVisible(false);
        }
    } else {
        for (auto &series : m_phasorFakeAxesSeriesList[0]) {
            series->setVisible(false);
        }
        for (auto &series : m_phasorFakeAxesSeriesList[1]) {
            series->setVisible(false);
        }
    }

    if (m_waveformCheckBox->isChecked()) {
        bool which = !m_phasorCheckBox->isChecked();
        for (auto &series : m_waveformFakeAxesSeriesList[which]) {
            series->setVisible(true);
        }
        for (auto &series : m_waveformFakeAxesSeriesList[which ^ 1]) {
            series->setVisible(false);
        }
    } else {
        for (auto &series : m_waveformFakeAxesSeriesList[0]) {
            series->setVisible(false);
        }
        for (auto &series : m_waveformFakeAxesSeriesList[1]) {
            series->setVisible(false);
        }
    }

    /// Replace points
    if (m_phasorCheckBox->isChecked()) {
        for (USize i = 0; i < NumChannels; ++i) {
            m_phasorSeriesList[i]->replace(m_phasorPointsList[i]);
        }
    }
    if (m_waveformCheckBox->isChecked()) {
        for (USize i = 0; i < NumChannels; ++i) {
            m_waveformSeriesList[i]->replace(m_waveformPointsList[i]);
        }
    }
    if (m_phasorCheckBox->isChecked() && m_phasorCheckBox->isChecked()
        && m_waveformCheckBox->isChecked()) {
        for (USize i = 0; i < NumChannels; ++i) {
            m_connectorSeriesList[i]->replace(m_connectorPointsList[i]);
        }
    }

    /// Update status and frequency labels
    if (isSimulating()) {
        m_statusLabel->setText(QStringLiteral("<strong>SIMULATING ROTATION</strong>"));
        m_frequencyLabel->setText(
                QStringLiteral("<strong>")
                + QString::number(SimulationFrequencyOptions[m_simulationFrequencyIndex])
                + QStringLiteral(" Hz</strong>"));
        auto palette = m_statusLabel->palette();
        palette.setColor(QPalette::Highlight, QStringLiteral("#808080"));
        m_statusLabel->setPalette(palette);
    } else {
        m_statusLabel->setText(QStringLiteral("<strong>LIVE</strong>"));
        m_frequencyLabel->setText(QStringLiteral("<strong>") + QString::number(est.freq, 'f', 1)
                                  + QStringLiteral(" Hz</strong>"));
        auto palette = m_statusLabel->palette();
        palette.setColor(QPalette::Highlight, QStringLiteral("#22bb45"));
        m_statusLabel->setPalette(palette);
    }

    /// Update table
    if (isSimulating()) {
        for (USize p = 0; p < NumPhases; ++p) {
            const auto &[vIdx, iIdx] = SignalPhasePairs[p];
            const auto &vAmpli = m_plotAmplitudes[vIdx];
            const auto &iAmpli = m_plotAmplitudes[iIdx];
            const auto &vPhase = m_plotPhaseDiffs[vIdx] * 180 / M_PI;
            const auto &iPhase = m_plotPhaseDiffs[iIdx] * 180 / M_PI;

            auto vPhasorText = QStringLiteral("<pre><strong>%1</strong><small>V ∠ "
                                              "</small><strong>%2</strong><small>°</small></pre>")
                                       .arg(vAmpli, 6, 'f', 1, ' ')
                                       .arg(vPhase, 6, 'f', 1, ' ');
            auto iPhasorText = QStringLiteral("<pre><strong>%1</strong><small>A ∠ "
                                              "</small><strong>%2</strong><small>°</small></pre>")
                                       .arg(iAmpli, 6, 'f', 1, ' ')
                                       .arg(iPhase, 6, 'f', 1, ' ');

            m_phasorLabels[vIdx]->setText(vPhasorText);
            m_phasorLabels[iIdx]->setText(iPhasorText);
        }
    } else {
        for (USize p = 0; p < NumPhases; ++p) {
            const auto &[vIdx, iIdx] = SignalPhasePairs[p];
            const auto &vAmpli = m_plotAmplitudes[vIdx];
            const auto &iAmpli = m_plotAmplitudes[iIdx];
            const auto &vPhase = m_plotPhaseDiffs[vIdx] * 180 / M_PI;
            const auto &iPhase = m_plotPhaseDiffs[iIdx] * 180 / M_PI;
            auto diff = std::abs(vPhase - iPhase);
            const auto &phaseDiff = std::min(diff, 360 - diff);

            auto vPhasorText = QStringLiteral("<pre><strong>%1</strong><small>V ∠ "
                                              "</small><strong>%2</strong><small>°</small></pre>")
                                       .arg(vAmpli, 6, 'f', 1, ' ')
                                       .arg(vPhase, 6, 'f', 1, ' ');
            auto iPhasorText = QStringLiteral("<pre><strong>%1</strong><small>A ∠ "
                                              "</small><strong>%2</strong><small>°</small></pre>")
                                       .arg(iAmpli, 6, 'f', 1, ' ')
                                       .arg(iPhase, 6, 'f', 1, ' ');
            auto phaseDiffText = QStringLiteral("<pre><strong>%1</strong><small>°</small></pre>")
                                         .arg(phaseDiff, 6, 'f', 1, ' ');

            m_phasorLabels[vIdx]->setText(vPhasorText);
            m_phasorLabels[iIdx]->setText(iPhasorText);
            m_phaseDiffLabels[p]->setText(phaseDiffText);
        }
    }
}
