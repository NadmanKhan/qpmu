#include "phasor_view.h"

QIcon circleIcon(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);

    painter.end();

    return QIcon(pixmap);
}

PhasorView::PhasorView(QTimer *updateTimer, Worker *worker, QWidget *parent)
    : QWidget(parent), m_worker(worker)
{
    assert(updateTimer != nullptr);
    assert(worker != nullptr);

    hide();

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
    m_table->setColumnCount(2);
    m_table->horizontalHeader()->hide();

    auto hBoxLayout = new QHBoxLayout();
    hBoxLayout->addWidget(chartView);
    hBoxLayout->addWidget(m_table);

    setLayout(hBoxLayout);

    // update every 400 ms (2.5 fps)
    m_timeoutTarget = 400 / updateTimer->interval();
    Q_ASSERT(m_timeoutTarget > 0);
    m_timeoutCounter = 0;
    connect(updateTimer, &QTimer::timeout, this, &PhasorView::update);

    for (int i = 0; i < nsignals; ++i) {
        const auto &[nameCStr, colorHexCStr, _type] = listSignalInfoModel[i];
        auto name = QString(nameCStr.data());
        auto color = QColor(colorHexCStr.data());

        auto lineSeries = new QLineSeries();
        m_listLineSeries.append(lineSeries);
        m_listLineSeriesPoints.push_back({});

        lineSeries->setName(name);
        lineSeries->setPen(QPen(color, 2.5));
        lineSeries->setUseOpenGL(true);

        // Phasor line goes
        // origin > tip > left-arm > tip > right-arm
        m_listLineSeriesPoints[i].append({ 0, 0 }); // origin
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow left-arm end (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow right-arm end (gets updated)

        lineSeries->replace(m_listLineSeriesPoints[i]);

        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisAngular);
        lineSeries->attachAxis(axisRadial);

        for (int j : { 0, 1 }) {
            auto tableItem = new QTableWidgetItem();
            tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

            tableItem->setFlags(Qt::NoItemFlags);
            tableItem->setFont(QFont(QStringLiteral("monospace")));
            tableItem->setForeground(QBrush(QColor(QStringLiteral("black"))));
            if (i & 1)
                tableItem->setBackground(QColor(QStringLiteral("lightGray")));
            m_table->setItem(i, j, tableItem);
        }
        auto vheaderItem = new QTableWidgetItem(circleIcon(color, 10), name);
        vheaderItem->setFlags(Qt::NoItemFlags);
        vheaderItem->setSizeHint(QSize(35, m_table->rowHeight(i)));
        m_table->setVerticalHeaderItem(i, vheaderItem);
    }

    m_table->setColumnWidth(0, 60);
    m_table->setColumnWidth(1, 60);
    int totalWidth = m_table->verticalHeader()->width();
    for (int i = 0; i < m_table->columnCount(); ++i) {
        totalWidth += m_table->columnWidth(i);
    }
    totalWidth += m_table->verticalScrollBar()->height();
    m_table->setMinimumWidth(totalWidth);
    m_table->setMaximumWidth(totalWidth);
    m_table->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

constexpr double polarChartAngle(double angle)
{
    return fmod(450 - angle, 360);
}

void PhasorView::update()
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
        m_listLineSeriesPoints[i][0] = { polars[i].x(), 0.015 };
        m_listLineSeriesPoints[i][1] = m_listLineSeriesPoints[i][3] = polars[i];

        { // create the arrow: two arms spreading out equal amounts
            qreal spread = 0.04;
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
        m_table->item(i, 0)->setText(QString::number(amplitudes[i], 'f', 1)
                                     + (listSignalInfoModel[i].signalType == SignalTypeVoltage
                                                ? QStringLiteral(" V")
                                                : QStringLiteral(" A")));
        m_table->item(i, 1)->setText(
                QStringLiteral("%1").arg(phaseDiff, 0, 'f', 1, QLatin1Char('0'))
                + QStringLiteral("°"));
        m_listLineSeries[i]->replace(m_listLineSeriesPoints[i]);
    }
}
