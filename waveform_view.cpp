#include "waveform_view.h"

WaveformView::WaveformView(Worker *worker, QWidget *parent) : QWidget(parent), m_worker(worker)
{
    m_axisTime = new QValueAxis();
    m_axisTime->setTitleText(QStringLiteral("Time (ms)"));
    m_axisTime->setRange(0, 100);
    m_axisTime->setTickCount(11);

    m_axisVoltage = new QValueAxis();
    m_axisVoltage->setTitleText(QStringLiteral("Voltage (V)"));
    m_axisVoltage->setRange(-100, 100);
    m_axisVoltage->setTickCount(11);

    m_axisCurrent = new QValueAxis();
    m_axisCurrent->setTitleText(QStringLiteral("Current (I)"));
    m_axisCurrent->setRange(-100, 100);
    m_axisCurrent->setTickCount(11);

    auto chart = new QChart();
    chart->addAxis(m_axisTime, Qt::AlignBottom);
    chart->addAxis(m_axisVoltage, Qt::AlignLeft);
    chart->addAxis(m_axisCurrent, Qt::AlignRight);
    //    chart->legend()->setAlignment(Qt::AlignLeft);

    auto chartView = new QChartView(this);
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->show();

    auto hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(chartView);
    setLayout(hBoxLayout);

    m_timer.setParent(this);
    m_timer.setInterval(400);
    connect(&m_timer, &QTimer::timeout, this, &WaveformView::updateSeries);

    for (int i = 0; i < nsignals; ++i) {
        const auto &[name, colorHex, signalType] = listSignalInfoModel[i];

        auto splineSeries = new QSplineSeries();
        m_listSplineSeries.append(splineSeries);
        m_listSplineSeriesPoints.push_back({});

        splineSeries->setName(name);
        splineSeries->setPen(QPen(QColor(colorHex), 2));
        splineSeries->setUseOpenGL(true);

        chart->addSeries(splineSeries);
        splineSeries->attachAxis(m_axisTime);
        splineSeries->attachAxis(signalType == SignalTypeVoltage ? m_axisVoltage : m_axisCurrent);
    }

    m_timer.start();
}

void WaveformView::updateSeries()
{
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
            auto y = amplitudes[i] * cos(2 * M_PI * frequencies[0] * t + phaseDiffs[i]);
            m_listSplineSeriesPoints[i][j] = QPointF(j * tDelta, y);
        }
    }

    m_axisVoltage->setRange(-vmax, +vmax);
    m_axisCurrent->setRange(-imax, +imax);
    //    qDebug() << frequencies[0];
    for (int i = 0; i < nsignals; ++i) {
        m_listSplineSeries[i]->replace(m_listSplineSeriesPoints[i]);
    }
}
