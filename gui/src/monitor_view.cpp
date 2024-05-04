#include "monitor_view.h"
#include "qpmu/common.h"
#include <iostream>

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
    auto vbox = new QVBoxLayout(this);
    setLayout(vbox);

    /// ChartView
    auto chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    //    auto chartViewSizePolicy = chartView->sizePolicy();
    //    chartViewSizePolicy.setWidthForHeight(true);
    //    chartViewSizePolicy.setHeightForWidth(true);
    //    chartView->setSizePolicy(chartViewSizePolicy);
    chartView->setContentsMargins(QMargins(0, 0, 0, 0));
    vbox->addWidget(chartView);

    /// Chart
    auto chart = new QChart();
    chart->legend()->hide();
    auto chartSizePolicy = chart->sizePolicy();
    //    chartSizePolicy.setWidthForHeight(true);
    //    chartSizePolicy.setHeightForWidth(true);
    chart->setSizePolicy(chartSizePolicy);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->setContentsMargins(QMargins(0, 0, 0, 0));
    chart->setAnimationOptions(QChart::NoAnimation);
    chartView->setChart(chart);

    /// X-axis
    auto axisX = new QValueAxis(chart);
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setLabelsVisible(false);
    axisX->setRange(0, (PolarGraphWidth + RectGraphWidth + 2 * Margin + Spacing) * Scale);
    axisX->setTickCount(2);
    axisX->setVisible(false);

    /// Y-axis
    auto axisY = new QValueAxis(chart);
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setLabelsVisible(false);
    axisY->setRange(Scale * -(Margin + PolarGraphWidth / 2),
                    Scale * +(Margin + PolarGraphWidth / 2));
    axisY->setTickCount(2);
    axisY->setVisible(false);

    /// Fake axes
    auto makeFakeAxisSeries = [=](std::string type) {
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
        s->setPen(QPen(color, 2));
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
        auto xOffset = QPointF(Margin + PolarGraphWidth / 2, 0);
        auto circle = makeFakeAxisSeries("spline");
        for (int i = 0; i <= (360 / 10); ++i) {
            qreal theta = qreal(i) / (360.0 / 10) * (2 * M_PI);
            circle->append(Scale * (unitvector(theta) + xOffset));
        }
        for (int i = 0; i <= (360 / 60); ++i) {
            qreal theta = qreal(i) / (360.0 / 60) * (2 * M_PI);
            auto tick = makeFakeAxisSeries("line");
            auto pen = tick->pen();
            pen.setWidthF(pen.widthF() * 0.5);
            tick->setPen(pen);
            tick->append(Scale * xOffset);
            tick->append(Scale * (1.025 * unitvector(theta) + xOffset));
        }
    }
    { /// Rectangular graph axes
        auto ver = makeFakeAxisSeries("line");
        ver->append(
                Scale
                * QPointF(Margin + Spacing + PolarGraphWidth, -((PolarGraphWidth + Margin) / 2)));
        ver->append(
                Scale
                * QPointF(Margin + Spacing + PolarGraphWidth, +((PolarGraphWidth + Margin) / 2)));
        auto hor = makeFakeAxisSeries("line");
        hor->append(Scale * QPointF(Margin + Spacing + PolarGraphWidth, 0));
        hor->append(Scale * QPointF(Margin + Spacing + PolarGraphWidth + RectGraphWidth, 0));
    }

    /// Data
    for (SizeType i = 0; i < NumChannels; ++i) {
        QString name = Signals[i].name;
        QColor color(Signals[i].colorHex);
        QPen pen(color, 2, Qt::SolidLine);

        /// Phasor series
        auto phasorSeries = new QLineSeries(chart);
        phasorSeries->setName(name);
        phasorSeries->setPen(pen);
        chart->addSeries(phasorSeries);
        phasorSeries->attachAxis(axisX);
        phasorSeries->attachAxis(axisY);

        /// Waveform series
        auto waveformSeries = new QSplineSeries(chart);
        waveformSeries->setName(name);
        waveformSeries->setPen(pen);
        chart->addSeries(waveformSeries);
        waveformSeries->attachAxis(axisX);
        waveformSeries->attachAxis(axisY);

        /// Connector series
        auto connectorSeries = new QLineSeries(chart);
        connectorSeries->setName(name);
        auto connectorPen = QPen(color.lighter(), 0.8, Qt::CustomDashLine);
        connectorPen.setDashPattern(QVector<qreal>() << (Scale * 4) << (Scale * 2));
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

    /// Controls
    auto hbox = new QHBoxLayout(this);
    vbox->addLayout(hbox, 1);

    /// RadioButton to enable/disable connectors
    auto radioEnableConnectors = new QRadioButton("Connector lines", this);
    connect(radioEnableConnectors, &QRadioButton::toggled, [=](bool checked) {
        for (SizeType i = 0; i < NumChannels; ++i) {
            m_connectorSeriesList[i]->setVisible(checked);
        }
    });
    radioEnableConnectors->setChecked(false);
    hbox->addWidget(radioEnableConnectors);

    /// ComboBox
    auto labelSimulate = new QLabel("Simulation frequency: ", this);
    hbox->addWidget(labelSimulate);
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
    hbox->addWidget(m_comboSimulationFrequency);
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
        FloatType f = SimulationFrequencyOptions[m_comboSimulationFrequency->currentIndex()];
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
        m_phasorPointsList[i][1] = ampli * unitvector(phase);
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
        // if (Signals[i].type == SignalType::Current) {
        // }

        /// Offset the points to fit the designated areas
        for (SizeType j = 0; j < 5; ++j) {
            m_phasorPointsList[i][j] += QPointF(Margin + PolarGraphWidth / 2, 0);
        }
        for (SizeType j = 0; j < (NumPointsPerCycle * NumCycles + 1); ++j) {
            m_waveformPointsList[i][j] += QPointF(Margin + Spacing + PolarGraphWidth, 0);
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
