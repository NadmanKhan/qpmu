#include "waveform_view.h"

WaveformView::WaveformView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    m_axisTime = new QValueAxis();
    m_axisTime->setTitleText(QStringLiteral("Time (ms)"));
    m_axisTime->setRange(0, 100);
    m_axisTime->setTickCount(11);

    m_axisVoltage = new QValueAxis();
    m_axisVoltage->setTitleText(QStringLiteral("Voltage (V)"));
    m_axisVoltage->setRange(-100, 100);
    m_axisVoltage->setTickAnchor(0);
    m_axisVoltage->setTickInterval(1);
    m_axisVoltage->setTickType(QValueAxis::TicksDynamic);

    m_axisCurrent = new QValueAxis();
    m_axisCurrent->setTitleText(QStringLiteral("Current (I)"));
    m_axisCurrent->setRange(-100, 100);
    m_axisCurrent->setTickAnchor(0);
    m_axisCurrent->setTickInterval(1);
    m_axisCurrent->setTickType(QValueAxis::TicksDynamic);

    auto chart = new QChart();
    chart->addAxis(m_axisTime, Qt::AlignBottom);
    chart->addAxis(m_axisVoltage, Qt::AlignLeft);
    chart->addAxis(m_axisCurrent, Qt::AlignRight);
    auto chartLegend = chart->legend();
    chartLegend->setMarkerShape(QLegend::MarkerShapeCircle);

    auto chartView = new QChartView(this);
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->show();

    auto hbox = new QHBoxLayout(this);
    hbox->addWidget(chartView);
    setLayout(hbox);

    // update every 400 ms (2.5 fps)
    m_timeoutTarget = 400 / updateTimer->interval();
    Q_ASSERT(m_timeoutTarget > 0);
    m_timeoutCounter = 0;
    connect(updateTimer, &QTimer::timeout, this, &WaveformView::update);

    const auto colorWhite = QColor(QStringLiteral("white"));

    for (int i = 0; i < nsignals; ++i) {
        const auto &[nameCStr, colorHexCStr, signalType] = listSignalInfoModel[i];
        auto name = QString(nameCStr.data());
        auto color = QColor(colorHexCStr.data());

        auto splineSeries = new QSplineSeries();
        m_listSplineSeries.append(splineSeries);
        m_listSplineSeriesPoints.push_back({});
        splineSeries->setName(name);
        splineSeries->setPen(QPen(color, 2));
        splineSeries->setUseOpenGL(true);

        chart->addSeries(splineSeries);
        splineSeries->attachAxis(m_axisTime);
        splineSeries->attachAxis(signalType == SignalTypeVoltage ? m_axisVoltage : m_axisCurrent);

        auto marker = chartLegend->markers(splineSeries).at(0);
        connect(splineSeries, &QSplineSeries::visibleChanged, [=] {
            marker->setVisible(true);
            marker->setBrush(QBrush(splineSeries->isVisible() ? color : colorWhite));
        });
        connect(marker, &QLegendMarker::clicked, [=] {
            auto newIsVisible = !splineSeries->isVisible();
            splineSeries->setVisible(newIsVisible);
        });
    }
}

void WaveformView::update()
{
    ++m_timeoutCounter;
    m_timeoutCounter = m_timeoutCounter * bool(m_timeoutCounter != m_timeoutTarget);
    if (m_timeoutCounter != 0)
        return;
    if (!isVisible())
        return;

    std::array<std::complex<double>, nsignals> phasors;
    std::array<double, nsignals> frequencies;

    m_worker->getEstimations(phasors, frequencies);

    std::array<double, nsignals> phaseDiffs; // in radians
    std::array<double, nsignals> amplitudes;

    double phaseRef = std::arg(phasors[0]);
    for (int i = 0; i < nsignals; ++i) {
        phaseDiffs[i] = (std::arg(phasors[i]) - phaseRef);
        amplitudes[i] = std::abs(phasors[i]) / Worker::N;
    }

    std::array<QPointF, nsignals> polars;

    qreal vmax = 0;
    qreal imax = 0;
    for (int i = 0; i < nsignals; ++i) {
        if (listSignalInfoModel[i].signalType == SignalTypeVoltage) {
            vmax = qMax(vmax, amplitudes[i]);
        } else {
            imax = qMax(imax, amplitudes[i]);
        }
    }

    const qreal tFactor = 1.0 / 1000; // ms
    const qreal tDelta = 5;
    for (int i = 0; i < nsignals; ++i) {
        m_listSplineSeriesPoints[i].resize(21);
        for (int j = 0; j <= 20; ++j) {
            auto t = j * tDelta * tFactor;
            auto y = amplitudes[i] * cos(frequencies[0] * t + phaseDiffs[i]);
            m_listSplineSeriesPoints[i][j] = QPointF(j * tDelta, y);
        }
    }

    m_axisVoltage->setRange(-vmax, +vmax);
    m_axisVoltage->setTickInterval(vmax);
    m_axisCurrent->setRange(-imax, +imax);
    m_axisCurrent->setTickInterval(imax);
    //    qDebug() << QList<double>(frequencies.begin(), frequencies.end());
    for (int i = 0; i < nsignals; ++i) {
        m_listSplineSeries[i]->replace(m_listSplineSeriesPoints[i]);
    }
}
