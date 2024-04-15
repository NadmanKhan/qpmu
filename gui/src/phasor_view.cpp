#include "phasor_view.h"

constexpr double polarChartAngle(double angle)
{
    return fmod(450 - angle, 360);
}

QTableWidgetItem *newCellItem(int row = -1)
{
    auto item = new QTableWidgetItem();
    item->setFlags(Qt::NoItemFlags);
    item->setForeground(QBrush(QColor(QStringLiteral("black"))));
    item->setTextAlignment(Qt::AlignVCenter);
    item->setFont(QFont(QStringLiteral("monospace")));
    if (row >= 0) {
        item->setTextAlignment(item->textAlignment() | Qt::AlignRight);
        if (row & 1) {
            item->setBackground(QColorConstants::LightGray);
        }
    }
    item->setSizeHint(QSize(50, 25));
    return item;
}

QIcon circleIcon(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, size, size);
    painter.end();

    return QIcon(pixmap);
}

QIcon twoColorCircleIcon(const QColor &color1, const QColor &color2, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color1);
    painter.drawPie(0, 0, size, size, 90 * 16, 270 * 16);
    painter.setBrush(color2);
    painter.drawPie(0, 0, size, size, 0 * 16, -90 * 16);
    painter.drawPie(0, 0, size, size, 0 * 16, +90 * 16);
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
    chartView->show();

    m_table1 = new QTableWidget(this);
    m_table1->setRowCount(NUM_SIGNALS);
    m_table1->setColumnCount(2);
    m_table1->horizontalHeader()->hide();

    m_table2 = new QTableWidget(this);
    m_table2->setRowCount(NUM_PHASES);
    m_table2->setColumnCount(1);
    m_table2->horizontalHeader()->hide();

    // Clear cell selections because they are unwanted.
    // Can't find any way to turn them off, hence this workaround.
    connect(m_table1, &QTableWidget::currentCellChanged,
            [=](int row, int column) { m_table1->setCurrentCell(-1, -1); });
    connect(m_table2, &QTableWidget::currentCellChanged,
            [=](int row, int column) { m_table2->setCurrentCell(-1, -1); });

    auto vboxTables = new QVBoxLayout();
    vboxTables->addWidget(m_table1);
    vboxTables->addSpacing(6);
    vboxTables->addWidget(m_table2);

    auto hbox = new QHBoxLayout();
    setLayout(hbox);
    hbox->addWidget(chartView, 1);
    hbox->addLayout(vboxTables, 0);

    for (int i = 0; i < NUM_SIGNALS; ++i) {
        auto name = QString(signalInfoList[i].name);
        auto color = QColor(signalInfoList[i].colorHex);

        // add signal to chart

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

        // add signal to table 1

        auto vheader = newCellItem();
        vheader->setIcon(circleIcon(color, 10));
        vheader->setText(name);
        m_table1->setVerticalHeaderItem(i, vheader);

        m_table1->setItem(i, 0, newCellItem(i));
        m_table1->setItem(i, 1, newCellItem(i));
    }

    for (int i = 0; i < NUM_PHASES; ++i) {
        const auto &[vIndex, iIndex] = phaseIndexPairList[i];
        auto vName = QString(signalInfoList[vIndex].name);
        auto vColor = QColor(signalInfoList[vIndex].colorHex);
        auto iName = QString(signalInfoList[iIndex].name);
        auto iColor = QColor(signalInfoList[iIndex].colorHex);
        auto phaseLetter = signalInfoList[vIndex].phaseLetter;
        Q_ASSERT(phaseLetter == signalInfoList[iIndex].phaseLetter);

        auto vheader = newCellItem();
        vheader->setIcon(twoColorCircleIcon(vColor, iColor, 10));
        vheader->setText(QStringLiteral("Δθ%1").arg(phaseLetter));
        m_table2->setVerticalHeaderItem(i, vheader);

        m_table2->setItem(i, 0, newCellItem(i));
    }

    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_table1->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_table2->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    m_table1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table1->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table2->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table1->setFixedHeight(m_table1->verticalHeader()->length() + 2);
    m_table2->setFixedHeight(m_table2->verticalHeader()->length() + 2);

    // update every 400 ms (2.5 fps)
    m_timeoutTarget = 400 / updateTimer->interval();
    Q_ASSERT(m_timeoutTarget > 0);
    m_timeoutCounter = 0;
    connect(updateTimer, &QTimer::timeout, this, &PhasorView::update);
}

void PhasorView::update()
{
    ++m_timeoutCounter;
    m_timeoutCounter = m_timeoutCounter * bool(m_timeoutCounter != m_timeoutTarget);
    if (m_timeoutCounter != 0)
        return;
    if (!isVisible())
        return;

    std::array<std::complex<double>, NUM_SIGNALS> phasors;
    double omega;
    m_worker->getEstimations(phasors, omega);

    std::array<double, NUM_SIGNALS> phaseDiffs;
    std::array<double, NUM_SIGNALS> amplitudes;

    double phaseRef = std::arg(phasors[0]);
    for (int i = 0; i < NUM_SIGNALS; ++i) {
        phaseDiffs[i] = (std::arg(phasors[i]) - phaseRef) * 180 / M_PI;
        phaseDiffs[i] = fmod(phaseDiffs[i] + 360, 360);
        amplitudes[i] = std::abs(phasors[i]) / Worker::N;
    }

    std::array<QPointF, NUM_SIGNALS> polars;

    for (int i = 0; i < NUM_SIGNALS; ++i) {
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

    for (int i = 0; i < NUM_SIGNALS; ++i) {
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

    for (int i = 0; i < NUM_SIGNALS; ++i) {
        auto phaseDiff = phaseDiffs[i];
        if (phaseDiff > 180)
            phaseDiff -= 360;
        m_table1->item(i, 0)->setText(QString::number(amplitudes[i], 'f', 1)
                                      + (signalInfoList[i].signalType == SignalTypeVoltage
                                                 ? QStringLiteral(" V")
                                                 : QStringLiteral(" A")));
        m_table1->item(i, 1)->setText(QString::number(phaseDiff, 'f', 1) + QStringLiteral("°"));
        m_listLineSeries[i]->replace(m_listLineSeriesPoints[i]);
    }

    for (int i = 0; i < NUM_PHASES; ++i) {
        const auto &[vIndex, iIndex] = phaseIndexPairList[i];
        m_table2->item(i, 0)->setText(
                QString::number((std::fmod(phaseDiffs[vIndex] - phaseDiffs[iIndex] + 360, 360)),
                                'f', 1)
                + QStringLiteral("°"));
    }
}
