#include "phasor_view.h"
#include "qpmu/common.h"
#include "util.h"

double polarChartAngle(double angle)
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
            item->setBackground(QColor(QStringLiteral("LightGray")));
        }
    }
    item->setSizeHint(QSize(50, 25));
    return item;
}

PhasorView::PhasorView(QWidget *parent) : QWidget(parent)
{
    using namespace qpmu;

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
    m_table1->setRowCount(NumChannels);
    m_table1->setColumnCount(2);
    m_table1->horizontalHeader()->hide();

    m_table2 = new QTableWidget(this);
    m_table2->setRowCount(NumPhases);
    m_table2->setColumnCount(1);
    m_table2->horizontalHeader()->hide();

    // Clear cell selections because they are unwanted.
    // Can't find any way to turn them off, hence this workaround.
    connect(m_table1, &QTableWidget::currentCellChanged, [=] { m_table1->setCurrentCell(-1, -1); });
    connect(m_table2, &QTableWidget::currentCellChanged, [=] { m_table2->setCurrentCell(-1, -1); });

    auto vboxTables = new QVBoxLayout();
    vboxTables->addWidget(m_table1);
    vboxTables->addSpacing(6);
    vboxTables->addWidget(m_table2);

    auto hbox = new QHBoxLayout();
    setLayout(hbox);
    hbox->addWidget(chartView, 1);
    hbox->addLayout(vboxTables, 0);

    for (USize i = 0; i < NumChannels; ++i) {
        auto name = QString(Signals[i].name);
        auto color = QColor(Signals[i].colorHex);

        auto lineSeries = new QLineSeries();
        m_listLineSeries.append(lineSeries);
        m_listLineSeriesPoints.push_back({});

        lineSeries->setName(name);
        lineSeries->setPen(QPen(color, 2.5));
        lineSeries->setUseOpenGL(true);

        // Lines sesries points to draw a phasor arrow:
        // [origin, tip, left-arm, tip, right-arm]
        m_listLineSeriesPoints[i].append({ 0, 0 }); // origin
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow left-arm end (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (gets updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow right-arm end (gets updated)

        lineSeries->replace(m_listLineSeriesPoints[i]);

        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisAngular);
        lineSeries->attachAxis(axisRadial);

        auto vheader = newCellItem();
        vheader->setIcon(circlePixmap(color, 10));
        vheader->setText(name);
        m_table1->setVerticalHeaderItem(i, vheader);

        m_table1->setItem(i, 0, newCellItem(i));
        m_table1->setItem(i, 1, newCellItem(i));
    }

    for (USize i = 0; i < NumPhases; ++i) {
        const auto &[vIndex, iIndex] = SignalPhasePairs[i];
        auto vName = QString(Signals[vIndex].name);
        auto vColor = QColor(Signals[vIndex].colorHex);
        auto iName = QString(Signals[iIndex].name);
        auto iColor = QColor(Signals[iIndex].colorHex);
        auto phaseId = signal_phase_char(Signals[vIndex]);
        Q_ASSERT(phaseId == signal_phase_char(Signals[iIndex]));

        auto vheader = newCellItem();
        vheader->setIcon(twoColorCirclePixmap(vColor, iColor, 10));
        vheader->setText(QStringLiteral("Δθ%1").arg(phaseId));
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
    m_timeoutTarget = 400 / APP->updateTimer()->interval();
    Q_ASSERT(m_timeoutTarget > 0);
    m_timeoutCounter = 0;
    connect(APP->updateTimer(), &QTimer::timeout, this, &PhasorView::update);
}

void PhasorView::update()
{
    ++m_timeoutCounter;
    m_timeoutCounter = m_timeoutCounter * bool(m_timeoutCounter != m_timeoutTarget);
    if (m_timeoutCounter != 0)
        return;
    if (!isVisible())
        return;

    using namespace qpmu;

    Estimation est;
    APP->worker()->getEstimations(est);

    std::array<FloatType, NumChannels> phaseDiffs;
    std::array<FloatType, NumChannels> amplitudes;

    FloatType phaseRef = est.phasor_ang[0];
    for (USize i = 0; i < NumChannels; ++i) {
        phaseDiffs[i] = (est.phasor_ang[i] - phaseRef) * (180 / M_PI);
        phaseDiffs[i] = std::fmod(phaseDiffs[i] + 360, 360);
        amplitudes[i] = est.phasor_mag[i];
    }

    std::array<QPointF, NumChannels> polars;

    for (USize i = 0; i < NumChannels; ++i) {
        polars[i] = QPointF(polarChartAngle(phaseDiffs[i]), amplitudes[i]);
    }

    for (USize t : { 0, 3 }) {
        FloatType ymax = 0;
        for (USize i = 0; i < 3; ++i) {
            ymax = std::max(ymax, amplitudes[t + i]);
        }
        for (USize i = 0; i < 3; ++i) {
            polars[t + i].setY(amplitudes[t + i] / ymax);
        }
    }

    for (USize i = 0; i < NumChannels; ++i) {
        m_listLineSeriesPoints[i][0] = { polars[i].x(), 0.02 };
        m_listLineSeriesPoints[i][1] = m_listLineSeriesPoints[i][3] = polars[i];

        { // create the arrow: two arms spreading out equal amounts
            FloatType spread = 0.04;
            FloatType distFromOrigin = polars[i].y() - (2 * spread);
            FloatType angle = std::atan2(spread, distFromOrigin);
            FloatType h = distFromOrigin / std::cos(angle);
            angle *= (180 / M_PI);

            m_listLineSeriesPoints[i][2] = QPointF(polars[i].x() + angle, h);
            m_listLineSeriesPoints[i][4] = QPointF(polars[i].x() - angle, h);
        }
    }

    for (USize i = 0; i < NumChannels; ++i) {
        auto phaseDiff = phaseDiffs[i];
        if (phaseDiff > 180)
            phaseDiff -= 360;
        m_table1->item(i, 0)->setText(QString::number(amplitudes[i], 'f', 1)
                                      + signal_unit_char(Signals[i]));
        m_table1->item(i, 1)->setText(QString::number(phaseDiff, 'f', 1) + "°");
        m_listLineSeries[i]->replace(m_listLineSeriesPoints[i]);
    }

    for (USize i = 0; i < NumPhases; ++i) {
        const auto &[vIndex, iIndex] = SignalPhasePairs[i];
        m_table2->item(i, 0)->setText(
                QString::number((std::fmod(phaseDiffs[vIndex] - phaseDiffs[iIndex] + 360, 360)),
                                'f', 1)
                + "°");
    }
}
