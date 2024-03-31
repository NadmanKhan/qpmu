#include "phasorview.h"
#include <tuple>

PhasorView::PhasorView(Worker *worker) : m_worker(worker)
{

    auto axisAngular = new QCategoryAxis();
    axisAngular->setRange(0, 360);
    axisAngular->setStartValue(0);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisAngular->append("0°", 90);
    axisAngular->append("30°", 60);
    axisAngular->append("60°", 30);
    axisAngular->append("90°", 0);
    axisAngular->append("120°", 330);
    axisAngular->append("150°", 300);
    axisAngular->append("180°", 270);
    axisAngular->append("-150°", 240);
    axisAngular->append("-120°", 210);
    axisAngular->append("-90°", 180);
    axisAngular->append("-60°", 150);
    axisAngular->append("-30°", 120);

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
        auto s = new QLineSeries();
        s->setName(name);
        s->setColor(color);
        s->setUseOpenGL(true);
        s->append(0, 0);
        s->append(0, 0);
        chart->addSeries(s);
        s->attachAxis(axisAngular);
        s->attachAxis(axisRadial);
        m_series.append(s);
    }

    setChart(chart);
    show();
    setVisible(true);

    auto timer = new QTimer(this);
    timer->setInterval(100);
    connect(timer, &QTimer::timeout, this, &PhasorView::updateSeries);
    timer->start();
}

QPointF phasorToPoint(const std::complex<double> phasor)
{
    auto phase = fmod(450 - (std::arg(phasor) * (180 / M_PI)), 360);
    return QPointF(phase, std::abs(phasor));
}

void PhasorView::updateSeries()
{
    QList<std::complex<double>> phasors = { 0, 0, 0, 0, 0, 0 };
    double frequency;
    m_worker->getPhasors(phasors, frequency);
    auto p1 = phasorToPoint(phasors[0]);
    QList<QPointF> points;
    for (int i = 0; i < 6; ++i) {
        auto p = phasorToPoint(phasors[i]);
        p.setX(fmod(p1.x() - p.x() + 360, 360));
        points.append(p);
    }
    for (int i = 0; i < 6; ++i) {
        m_series[i]->replace(1, points[i]);
    }
}
