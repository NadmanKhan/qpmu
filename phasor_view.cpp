#include "phasor_view.h"

PhasorView::PhasorView(Worker *worker, QWidget *parent) : QWidget(parent), m_worker(worker)
{
    auto axisAngular = new QCategoryAxis();
    axisAngular->setStartValue(0);
    axisAngular->setRange(0, 360);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    axisAngular->append("90°", 0);
    axisAngular->append("60°", 30);
    axisAngular->append("30°", 60);
    axisAngular->append("0°", 90);
    axisAngular->append("-30°", 120);
    axisAngular->append("-60°", 150);
    axisAngular->append("-90°", 180);
    axisAngular->append("-120°", 210);
    axisAngular->append("-150°", 240);
    axisAngular->append("180°", 270);
    axisAngular->append("150°", 300);
    axisAngular->append("120°", 330);

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
    m_table->setRowCount((int)signalInfoListModel.size());
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({ "Signal", "Phase diff.", "Magnitude" });
    m_table->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 80);
    m_table->setColumnWidth(2, 80);

    auto hboxLayout = new QHBoxLayout();
    hboxLayout->addWidget(chartView);
    hboxLayout->addWidget(m_table);
    setLayout(hboxLayout);

    auto timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &PhasorView::updateSeries);

    for (int i = 0; i < (int)signalInfoListModel.size(); ++i) {
        const auto &[name, color] = signalInfoListModel[i];

        auto lineSeries = new QLineSeries();
        m_listLineSeries.append(lineSeries);
        lineSeries->setName(name);
        lineSeries->setPen(QPen(QColor(color), 2));
        lineSeries->setUseOpenGL(true);

        m_listLineSeriesPoints.push_back({});
        m_listLineSeriesPoints[i].append({ 0, 0 }); // origin
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow left arm (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // tip (to be updated)
        m_listLineSeriesPoints[i].append({ 0, 0 }); // arrow right arm (to be updated)

        lineSeries->replace(m_listLineSeriesPoints[i]);

        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisAngular);
        lineSeries->attachAxis(axisRadial);

        for (int j : { 0, 1, 2 }) {
            auto tableItem = new QTableWidgetItem();
            tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
            if (j > 0)
                tableItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

            tableItem->setFont(QFont("monospace"));
            tableItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_table->setItem(i, j, tableItem);
        }

        m_table->item(i, 0)->setText(name);
        m_table->setVerticalHeaderItem(i, new QTableWidgetItem());
        m_table->verticalHeaderItem(i)->setBackground(QColor(color));
        m_table->verticalHeaderItem(i)->setSizeHint(QSize(10, 0));
    }

    timer->start();
}

constexpr double polarChartAngle(double angle)
{
    return fmod(450 - angle, 360);
}

void PhasorView::updateSeries()
{
    std::array<std::complex<double>, signalInfoListModel.size()> phasors = { 0, 0, 0, 0, 0, 0 };
    double frequency;

    m_worker->getPhasors(phasors, frequency);

    std::array<double, signalInfoListModel.size()> phaseDiffs;
    std::array<double, signalInfoListModel.size()> magnitudes;

    double phaseRef = std::arg(phasors[0]);
    for (int i = 0; i < (int)signalInfoListModel.size(); ++i) {
        phaseDiffs[i] = (std::arg(phasors[i]) - phaseRef) * 180 / M_PI;
        phaseDiffs[i] = fmod(phaseDiffs[i] + 360, 360);
        magnitudes[i] = std::abs(phasors[i]) / 24;
    }

    std::array<QPointF, signalInfoListModel.size()> polars;

    for (int i = 0; i < (int)signalInfoListModel.size(); ++i) {
        polars[i] = QPointF(polarChartAngle(phaseDiffs[i]), magnitudes[i]);
    }

    for (int t : { 0, 3 }) {
        qreal ymax = 0;
        for (int i = 0; i < 3; ++i) {
            ymax = qMax(ymax, magnitudes[t + i]);
        }
        for (int i = 0; i < 3; ++i) {
            polars[t + i].setY(magnitudes[t + i] / ymax);
        }
    }

    for (int i = 0; i < (int)signalInfoListModel.size(); ++i) {
        m_listLineSeriesPoints[i][1] = m_listLineSeriesPoints[i][3] = polars[i];

        { // create the arrow with two arms spreading out
            qreal spread = 0.03;
            qreal orthoDistOrigin = polars[i].y() - (2 * spread);
            qreal angle = atan2(spread, orthoDistOrigin);
            qreal h = orthoDistOrigin / cos(angle);
            angle *= (180 / M_PI);

            m_listLineSeriesPoints[i][2] = QPointF(polars[i].x() + angle, h);
            m_listLineSeriesPoints[i][4] = QPointF(polars[i].x() - angle, h);
        }
    }

    for (int i = 0; i < (int)signalInfoListModel.size(); ++i) {
        auto phaseDiff = phaseDiffs[i];
        if (phaseDiff > 180)
            phaseDiff -= 360;
        m_table->item(i, 1)->setText(QStringLiteral("%1").arg(phaseDiff,
                                                              (5 + bool(phaseDiff < -1e-9)), 'f', 1,
                                                              QLatin1Char('0'))
                                     + QStringLiteral("°"));
        m_table->item(i, 2)->setText(QString::number(magnitudes[i], 'f', 1) + "");
        m_listLineSeries[i]->replace(m_listLineSeriesPoints[i]);
    }
}
