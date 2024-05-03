#include "waveform_view.h"
#include "qpmu/common.h"

WaveformView::WaveformView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    using namespace qpmu;
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    m_axisTime = new QValueAxis();
    m_axisTime->setTitleText(QStringLiteral("Time (ms)"));
    m_axisTime->setRange(0, 50);
    m_axisTime->setTickCount(NumChannels + 2);
    m_axisTime->setLabelsVisible(false);

    m_axisVoltage = new QValueAxis();
    m_axisVoltage->setTitleText(QStringLiteral("Voltage (V)"));
    m_axisVoltage->setRange(-100, 100);
    m_axisVoltage->setTickAnchor(0);
    m_axisVoltage->setTickInterval(1);
    m_axisVoltage->setTickType(QValueAxis::TicksDynamic);

    m_axisCurrent = new QValueAxis();
    m_axisCurrent->setTitleText(QStringLiteral("Current (A)"));
    m_axisCurrent->setRange(-100, 100);
    m_axisCurrent->setTickAnchor(0);
    m_axisCurrent->setTickInterval(1);
    m_axisCurrent->setTickType(QValueAxis::TicksDynamic);

    m_chart = new QChart();
    m_chart->addAxis(m_axisTime, Qt::AlignBottom);
    m_chart->addAxis(m_axisVoltage, Qt::AlignLeft);
    m_chart->addAxis(m_axisCurrent, Qt::AlignRight);
    auto chartLegend = m_chart->legend();
    chartLegend->setMarkerShape(QLegend::MarkerShapeCircle);

    auto chartView = new QChartView(this);
    chartView->setChart(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartLegend->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->show();

    auto hbox = new QHBoxLayout(this);
    hbox->addWidget(chartView);
    setLayout(hbox);

    const auto colorWhite = QColor(QStringLiteral("white"));

    for (SizeType i = 0; i < NumChannels; ++i) {
        auto name = QString(Signals[i].name);
        auto color = QColor(Signals[i].colorHex);

        auto splineSeries = new QSplineSeries();
        m_listSplineSeries.append(splineSeries);
        m_listSplineSeriesPoints.push_back(
                QVector<QPointF>(PlotPointsPerCycle * PlotNumCycles + 1));
        splineSeries->setName(name);
        splineSeries->setPen(QPen(color, 2));
        splineSeries->setUseOpenGL(true);

        m_chart->addSeries(splineSeries);
        splineSeries->attachAxis(m_axisTime);
        splineSeries->attachAxis(signal_is_voltage(Signals[i]) ? m_axisVoltage : m_axisCurrent);

        auto marker = chartLegend->markers(splineSeries).at(0);
        auto markerPenVisible = QPen(QBrush(color), 5);
        auto markerPenNotVisible = QPen(QBrush(colorWhite), 2);
        marker->setPen(markerPenVisible);

        connect(splineSeries, &QSplineSeries::visibleChanged, [=] {
            marker->setVisible(true);
            marker->setPen(splineSeries->isVisible() ? markerPenVisible : markerPenNotVisible);
        });
        connect(marker, &QLegendMarker::clicked, [=] {
            auto newIsVisible = !splineSeries->isVisible();
            splineSeries->setVisible(newIsVisible);
        });
    }

    // update every 400 ms (2.5 fps)
    m_timeoutTarget = 400 / updateTimer->interval();
    Q_ASSERT(m_timeoutTarget > 0);
    m_timeoutCounter = 0;
    connect(updateTimer, &QTimer::timeout, this, &WaveformView::update);
    update();
}

void WaveformView::update()
{
    using namespace qpmu;

    ++m_timeoutCounter;
    m_timeoutCounter = m_timeoutCounter * bool(m_timeoutCounter != m_timeoutTarget);
    if (m_timeoutCounter != 0)
        return;
    if (!isVisible())
        return;

    Measurement msr;
    m_worker->getMeasurement(msr);

    const auto &phasors = msr.phasors;
    const auto &freq = msr.freq;

    std::array<FloatType, NumChannels> phaseDiffs; // in radians
    std::array<FloatType, NumChannels> amplitudes;

    FloatType phaseRef = std::arg(phasors[0]);
    for (SizeType i = 0; i < NumChannels; ++i) {
        phaseDiffs[i] = (std::arg(phasors[i]) - phaseRef);
        amplitudes[i] = std::abs(phasors[i]);
    }

    FloatType vmax = 0;
    FloatType imax = 0;
    for (SizeType i = 0; i < NumChannels; ++i) {
        if (signal_is_voltage(Signals[i])) {
            vmax = std::max(vmax, amplitudes[i]);
        } else {
            imax = std::max(imax, amplitudes[i]);
        }
    }

    FloatType timePeriod = 1.0 / freq;
    FloatType timeDelta = (timePeriod / PlotPointsPerCycle);
    FloatType _2_pi_f = 2 * M_PI * freq;

    for (SizeType i = 0; i < NumChannels; ++i) {
        for (SizeType j = 0; j < (SizeType)m_listSplineSeriesPoints[i].size(); ++j) {
            FloatType t = j * timeDelta;
            FloatType x = amplitudes[i] * std::cos(_2_pi_f * t + phaseDiffs[i]);
            m_listSplineSeriesPoints[i][j] = QPointF(t, x);
        }
    }

    m_axisTime->setRange(0, PlotNumCycles * timePeriod);
    m_axisVoltage->setRange(-vmax, +vmax);
    m_axisVoltage->setTickInterval(vmax);
    m_axisCurrent->setRange(-imax, +imax);
    m_axisCurrent->setTickInterval(imax);
    for (SizeType i = 0; i < NumChannels; ++i) {
        m_listSplineSeries[i]->replace(m_listSplineSeriesPoints[i]);
    }
}
