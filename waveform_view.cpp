#include "waveform_view.h"

WaveformView::WaveformView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

    m_axisTime = new QValueAxis();
    m_axisTime->setTitleText(QStringLiteral("Time (ms)"));
    m_axisTime->setRange(0, 50);
    m_axisTime->setTickCount(11);
    m_axisTime->setLabelFormat(QStringLiteral("%.0f"));

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

        m_chart->addSeries(splineSeries);
        splineSeries->attachAxis(m_axisTime);
        splineSeries->attachAxis(signalType == SignalTypeVoltage ? m_axisVoltage : m_axisCurrent);

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
    double frequency;

    m_worker->getEstimations(phasors, frequency);

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

    const qreal ω = 2 * M_PI * frequency;
    const qreal factor = ω * (0.001 /* because in ms */);
    const qreal tDelta = 5;
    const int npoints = 10; // excluding t=0

    for (int i = 0; i < nsignals; ++i) {
        m_listSplineSeriesPoints[i].resize(npoints + (1 /* for t=0 */));
        m_listSplineSeriesPoints[i][0] = QPointF(0, amplitudes[i] * cos(phaseDiffs[i]));
        for (int tIndex = 1; tIndex <= npoints; ++tIndex) {
            auto ωt = tIndex * tDelta * factor;
            m_listSplineSeriesPoints[i][tIndex] =
                    QPointF((tIndex * tDelta), amplitudes[i] * cos(ωt + phaseDiffs[i]));
        }
    }

    qDebug() << frequency;

    m_axisTime->setRange(0, npoints * tDelta);
    m_axisVoltage->setRange(-vmax, +vmax);
    m_axisVoltage->setTickInterval(vmax);
    m_axisCurrent->setRange(-imax, +imax);
    m_axisCurrent->setTickInterval(imax);
    for (int i = 0; i < nsignals; ++i) {
        m_listSplineSeries[i]->replace(m_listSplineSeriesPoints[i]);
    }
}
