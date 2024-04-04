#include "phasor_view.h"

PhasorView::PhasorView(Worker *worker, QWidget *parent) : QWidget(parent), m_worker(worker)
{
    assert(worker != nullptr);

    auto axisAngular = new QCategoryAxis();
    axisAngular->setStartValue(0);
    axisAngular->setRange(0, 360);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    axisAngular->append(QStringLiteral("90°"), 0);
    axisAngular->append(QStringLiteral("60°"), 30);
    axisAngular->append(QStringLiteral("30°"), 60);
    axisAngular->append(QStringLiteral("0°"), 90);
    axisAngular->append(QStringLiteral("-30°"), 120);
    axisAngular->append(QStringLiteral("-60°"), 150);
    axisAngular->append(QStringLiteral("-90°"), 180);
    axisAngular->append(QStringLiteral("-120°"), 210);
    axisAngular->append(QStringLiteral("-150°"), 240);
    axisAngular->append(QStringLiteral("180°"), 270);
    axisAngular->append(QStringLiteral("150°"), 300);
    axisAngular->append(QStringLiteral("120°"), 330);

    auto axisRadial = new QValueAxis();
    axisRadial->setRange(0, 1);
    axisRadial->setLabelsVisible(false);

    auto chart = new QPolarChart();
    chart->addAxis(axisAngular, QPolarChart::PolarOrientationAngular);
    chart->addAxis(axisRadial, QPolarChart::PolarOrientationRadial);
    chart->legend()->hide();

    auto chartView = new QChartView(this);
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->show();

    m_table = new QTableWidget(this);
    m_table->setRowCount(nsignals);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({ QStringLiteral("Signal"), QStringLiteral("Amplitude"),
                                         QStringLiteral("Phase diff.") });
    m_table->setColumnWidth(0, 45);
    m_table->setColumnWidth(1, 80);
    m_table->setColumnWidth(2, 80);
    m_table->setMinimumWidth(m_table->horizontalHeader()->length()
                             + m_table->verticalHeader()->width());
    m_table->setMaximumWidth(m_table->minimumWidth());
    m_table->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    auto hBoxLayout = new QHBoxLayout();
    hBoxLayout->addWidget(chartView);
    hBoxLayout->addWidget(m_table);

    setLayout(hBoxLayout);

    m_timer.setParent(this);
    m_timer.setInterval(400);
    connect(&m_timer, &QTimer::timeout, this, &PhasorView::updateSeries);

    for (int i = 0; i < nsignals; ++i) {
        const auto &[name, colorHex, _type] = listSignalInfoModel[i];

        auto lineSeries = new QLineSeries();
        m_listLineSeries.append(lineSeries);
        m_listLineSeriesPoints.push_back({});

        lineSeries->setName(name);
        lineSeries->setPen(QPen(QColor(colorHex), 2.5));
        lineSeries->setUseOpenGL(true);

        m_listLineSeriesPoints[i].append({ 0, 0 }); // origin
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow left-arm end (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow right-arm end (to be updated)

        lineSeries->replace(m_listLineSeriesPoints[i]);

        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisAngular);
        lineSeries->attachAxis(axisRadial);

        for (int j : { 0, 1, 2 }) {
            auto tableItem = new QTableWidgetItem();
            if (j == 0)
                tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
            else
                tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

            tableItem->setFont(QFont(QStringLiteral("monospace")));
            tableItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
            if (i & 1)
                tableItem->setBackground(QColor(QStringLiteral("lightGray")));
            m_table->setItem(i, j, tableItem);
            m_table->setRowHeight(i, 65);
        }

        m_table->item(i, 0)->setText(name);
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem());
        m_table->verticalHeaderItem(i)->setBackground(QColor(colorHex));
        m_table->verticalHeaderItem(i)->setSizeHint(QSize(10, 0));
    }
    m_table->horizontalHeader()->hide();

    m_timer.start();
}

constexpr double polarChartAngle(double angle)
{
    return fmod(450 - angle, 360);
}

void PhasorView::updateSeries()
{
    std::array<std::complex<double>, nsignals> phasors;
    std::array<double, nsignals> frequencies;

    m_worker->getEstimations(phasors, frequencies);

    std::array<double, nsignals> phaseDiffs;
    std::array<double, nsignals> amplitudes;

    double phaseRef = std::arg(phasors[0]);
    for (int i = 0; i < nsignals; ++i) {
        phaseDiffs[i] = (std::arg(phasors[i]) - phaseRef) * 180 / M_PI;
        phaseDiffs[i] = fmod(phaseDiffs[i] + 360, 360);
        amplitudes[i] = std::abs(phasors[i]) / Worker::N;
    }

    std::array<QPointF, nsignals> polars;

    for (int i = 0; i < nsignals; ++i) {
        polars[i] = QPointF(polarChartAngle(phaseDiffs[i]), amplitudes[i]);
    }

    for (int t : { 0, 3 }) {
        qreal ymax = 0;
        for (int i = 0; i < 3; ++i) {
            ymax = qMax(ymax, amplitudes[t + i]);
        }
        for (int i = 0; i < 3; ++i) {
            polars[t + i].setY(amplitudes[t + i] / ymax);
        }
    }

    for (int i = 0; i < nsignals; ++i) {
        m_listLineSeriesPoints[i][1] = m_listLineSeriesPoints[i][3] = polars[i];

        { // create the arrow: two arms spreading out equal amounts
            qreal spread = 0.03;
            qreal distFromOrigin = polars[i].y() - (2 * spread);
            qreal angle = atan2(spread, distFromOrigin);
            qreal h = distFromOrigin / cos(angle);
            angle *= (180 / M_PI);

            m_listLineSeriesPoints[i][2] = QPointF(polars[i].x() + angle, h);
            m_listLineSeriesPoints[i][4] = QPointF(polars[i].x() - angle, h);
        }
    }

    for (int i = 0; i < nsignals; ++i) {
        auto phaseDiff = phaseDiffs[i];
        if (phaseDiff > 180)
            phaseDiff -= 360;
        m_table->item(i, 1)->setText(QString::number(amplitudes[i], 'f', 1)
                                     + (listSignalInfoModel[i].signalType == SignalTypeVoltage
                                                ? QStringLiteral(" V")
                                                : QStringLiteral(" A")));
        m_table->item(i, 2)->setText(
                QStringLiteral("%1").arg(phaseDiff, 0, 'f', 1, QLatin1Char('0'))
                + QStringLiteral("°"));
        m_listLineSeries[i]->replace(m_listLineSeriesPoints[i]);
    }
}
