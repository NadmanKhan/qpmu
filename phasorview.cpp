#include "phasorview.h"
#include <tuple>
#include <iostream>
#include <iomanip>
#include <cmath>

PhasorView::PhasorView(Worker *worker) : m_worker(worker)
{

    auto axisAngular = new QCategoryAxis();
    axisAngular->setStartValue(0);
    axisAngular->setRange(0, 360);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    //    axisAngular->setReverse(true);

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
    //    chart->setVisible(true);
    setRenderHint(QPainter::Antialiasing, true);

    auto model = QList<std::tuple<QString, QString>>{
        std::make_tuple(QStringLiteral("VA"), QStringLiteral("#ff0000")),
        std::make_tuple(QStringLiteral("VB"), QStringLiteral("#00ff00")),
        std::make_tuple(QStringLiteral("VC"), QStringLiteral("#0000ff")),
        std::make_tuple(QStringLiteral("IA"), QStringLiteral("#00ffff")),
        std::make_tuple(QStringLiteral("IB"), QStringLiteral("#ff00ff")),
        std::make_tuple(QStringLiteral("IC"), QStringLiteral("#ffff00"))
    };
    for (const auto item : model) {
        const auto &[name, color] = item;
        auto lineSeries = new QLineSeries();
        lineSeries->setName(name);
        lineSeries->setPen(QPen(QColor(color), 2));
        lineSeries->setUseOpenGL(true);
        lineSeries->append(0, 0);
        lineSeries->append(0, 0);
        lineSeries->append(0, 0);
        lineSeries->append(0, 0);
        lineSeries->append(0, 0);
        lineSeries->append(0, 0);
        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisAngular);
        lineSeries->attachAxis(axisRadial);
        m_lineSeries.append(lineSeries);

        //        auto scatterSeries = new QScatterSeries();
        //        scatterSeries->setName(name);
        //        scatterSeries->setMarkerSize(7);
        //        scatterSeries->setColor(color);
        //        scatterSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        //        scatterSeries->setUseOpenGL(true);
        //        scatterSeries->append(0, 0);
        //        chart->addSeries(scatterSeries);
        //        scatterSeries->attachAxis(axisAngular);
        //        scatterSeries->attachAxis(axisRadial);
        //        m_scatterSeries.append(scatterSeries);
    }

    setChart(chart);
    show();
    setVisible(true);

    auto timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &PhasorView::updateSeries);
    timer->start();
}

QPointF toPolarDeg(const std::complex<double> phasor)
{
    return { std::arg(phasor) * 180 / M_PI, std::abs(phasor) };
}

QPointF &fixForChart(QPointF &polarPoint)
{
    polarPoint.setX(fmod(450 - polarPoint.x(), 360));
    return polarPoint;
}

void PhasorView::updateSeries()
{
    std::array<std::complex<double>, 6> phasors = { 0, 0, 0, 0, 0, 0 };
    double frequency;
    m_worker->getPhasors(phasors, frequency);

    auto point0 = toPolarDeg(phasors[0]);
    std::array<QPointF, 6> points;

    for (int i = 0; i < 6; ++i) {
        points[i] = toPolarDeg(phasors[i]);
        points[i].setX(fmod(points[i].x() - point0.x() + 360, 360));
    }
    //    std::cout << std::fixed << std::setprecision(2);
    //    for (int i = 0; i < 6; ++i) {
    //        std::cout << "(" << points[i].x() << ", " << points[i].y() << ") ";
    //    }
    //    std::cout << '\n';

    for (int i = 0; i < 6; ++i) {
        fixForChart(points[i]);
    }

    for (int t : { 0, 3 }) {
        qreal ymax = 0;
        for (int i = 0; i < 3; ++i) {
            ymax = qMax(ymax, points[t + i].y());
        }
        for (int i = 0; i < 3; ++i) {
            points[t + i].setY(points[t + i].y() * 0.99 / ymax);
        }
    }

    for (int i = 0; i < 6; ++i) {
        //        m_lineSeries[i]->replace(0, QPointF(points[i].x(), 0.02));
        //        m_lineSeries[i]->replace(1, points[i]);

        auto seriesPoints = m_lineSeries[i]->points();
        seriesPoints[1] = seriesPoints[3] = seriesPoints[5] = points[i];

        const qreal l = 0.03;
        qreal alpha = atan2(l, points[i].y() - l);
        // cos(alpha) = (points[i].y() - l) / h
        qreal h = (points[i].y() - l) / cos(alpha);

        alpha *= (180 / M_PI);
        seriesPoints[2] = QPointF(points[i].x() + alpha, h);
        seriesPoints[4] = QPointF(points[i].x() - alpha, h);
        qDebug() << seriesPoints;
        m_lineSeries[i]->replace(seriesPoints);
    }
}
