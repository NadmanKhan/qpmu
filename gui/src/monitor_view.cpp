#include "monitor_view.h"
#include "qpmu/common.h"
#include <iostream>
#include <qabstractseries.h>
#include <qlineseries.h>
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
    auto outerHbox = new QHBoxLayout(this);
    setLayout(outerHbox);

    /// Visualization area layout
    auto visualVbox = new QVBoxLayout(this);
    outerHbox->addLayout(visualVbox);

    /// ChartView
    auto chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    visualVbox->addWidget(chartView);

    /// Chart
    auto chart = new QChart();
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);
    chartView->setChart(chart);

    /// X-axis
    auto axisX = new QValueAxis(chart);
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setLabelsVisible(false);
    axisX->setRange(0, (PolarGraphWidth + RectGraphWidth + Spacing) * Scale);
    axisX->setTickCount(2);
    axisX->setVisible(false);

    /// Y-axis
    auto axisY = new QValueAxis(chart);
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setLabelsVisible(false);
    axisY->setRange(Scale * -(PolarGraphWidth / 2), Scale * +(PolarGraphWidth / 2));
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
            const qreal radius =
                    qreal(radiusPoint) / 4 * PolarGraphWidth / 2 - bool(radiusPoint == 4) * 0.005;
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

    /// Data
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

    /// Layout for constrols (side panel)
    auto sideGrid = new QGridLayout(this);
    sideGrid->setContentsMargins(QMargins(0, 20, 0, 20));
    outerHbox->addLayout(sideGrid);

    /// CheckBoxes to enable/disable series
    {
        for (SignalType type : { SignalType::Voltage, SignalType::Current }) {

            /// Individual voltages or currents
            QList<QCheckBox *> checks;
            for (SizeType i = 0; i < NumChannels; ++i) {
                if (Signals[i].type == type) {
                    auto check = new QCheckBox(Signals[i].name, this);
                    connect(check, &QCheckBox::toggled, [=](bool checked) {
                        m_phasorSeriesList[i]->setVisible(checked);
                        m_waveformSeriesList[i]->setVisible(checked);
                        m_connectorSeriesList[i]->setVisible(checked);
                    });
                    check->setChecked(true);
                    sideGrid->addWidget(check, checks.size(), type);
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
            sideGrid->addWidget(checkAll, checks.size(), type);

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
    }

    /// RadioButton to enable/disable connectors
    auto radioEnableConnectors = new QRadioButton("Connector lines", this);
    connect(radioEnableConnectors, &QRadioButton::toggled, [=](bool checked) {
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_connectorSeriesList[i]->setVisible(m_phasorSeriesList[i]->isVisible() && checked);
        }
    });
    radioEnableConnectors->setChecked(true);
    sideGrid->addWidget(radioEnableConnectors, sideGrid->rowCount(), 0, 1, 2);

    /// ComboBox
    auto labelSimulate = new QLabel("Simulation frequency: ", this);
    sideGrid->addWidget(labelSimulate);
    m_comboSimulationFrequency = new QComboBox(this);
    {
        QStringList items;
        items << "Off";
        for (auto f : SimulationFrequencyOptions) {
            items << QString::number(f) + " Hz";
        }
        m_comboSimulationFrequency->addItems(items);
    }
    m_comboSimulationFrequency->setCurrentIndex(0);
    sideGrid->addWidget(m_comboSimulationFrequency);
}

void MonitorView::update()
{
    using namespace qpmu;

    if (!isVisible())
        return;

    std::array<FloatType, NumChannels> phaseDiffs; // in radians
    std::array<FloatType, NumChannels> amplitudes;

    Estimations est;
    m_worker->getEstimations(est);

    FloatType phaseRef = std::arg(est.phasors[0]);
    FloatType maxAmpli[2] = { 0, 0 };

    for (SizeType i = 0; i < NumChannels; ++i) {
        phaseDiffs[i] = std::arg(est.phasors[i]);
        amplitudes[i] = std::abs(est.phasors[i]);

        maxAmpli[Signals[i].type] = std::max(maxAmpli[Signals[i].type], amplitudes[i]);
    }

    for (SizeType i = 0; i < NumChannels; ++i) {
        phaseDiffs[i] -= phaseRef;
        amplitudes[i] /= maxAmpli[Signals[i].type];
    }

    if (m_comboSimulationFrequency->currentIndex() > 0) {
        /// simulating; nudge the phasors according to the chosen frequency
        FloatType f = SimulationFrequencyOptions[m_comboSimulationFrequency->currentIndex() - 1];
        FloatType delta = 2 * M_PI * f * UpdateIntervalMs / 1000;
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_plottedPhasors[i] *= std::polar(1.0, delta);
        }
    } else {
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_plottedPhasors[i] = std::polar(amplitudes[i], phaseDiffs[i]);
        }
    }

    for (SizeType i = 0; i < NumChannels; ++i) {
        auto phasor = m_plottedPhasors[i];
        auto phase = std::arg(phasor);
        auto ampli = std::abs(phasor);

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
        m_phasorPointsList[i][1] = ampli * 0.99 * unitvector(phase);
        m_phasorPointsList[i][2] = alphaHypotenuse * unitvector(phase + alpha);
        m_phasorPointsList[i][3] = m_phasorPointsList[i][1];
        m_phasorPointsList[i][4] = alphaHypotenuse * unitvector(phase - alpha);

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
}
