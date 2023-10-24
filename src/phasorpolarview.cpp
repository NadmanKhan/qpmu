#include "phasorpolarview.h"
#include <iostream>

PhasorPolarView::PhasorPolarView(QWidget* parent)
    : AbstractPhasorView(new QPolarChart(), parent) {

    auto chart = qobject_cast<QPolarChart*>(this->chart());

    /// create axes

    auto axisAngular = new QCategoryAxis(this);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisAngular->setRange(0, 360);
    for (qint16 angleOnAxis = 0; angleOnAxis < 360; angleOnAxis += 30) {
        auto label = QString::asprintf("%d&deg;", toAngleActual(angleOnAxis));
        axisAngular->append(label, angleOnAxis);
    }
    chart->addAxis(axisAngular, QPolarChart::PolarOrientationAngular);

    auto axisRadial = new QValueAxis(this);
    chart->addAxis(axisRadial, QPolarChart::PolarOrientationRadial);

    /// create series and add points

    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        auto s = new QLineSeries(this);
        chart->addSeries(s);

        s->setName(PMU::names[i]);
        s->setPen(QPen(QBrush(PMU::colors[i]), 2.0));
        s->attachAxis(axisAngular);
        s->attachAxis(axisRadial);
        addSeriesToControl(s, PMU::types[i]);
        s->append(0.0, 0.0);
        s->append(0.0, 0.0);
        m_series[i] = s;
    }

    connect(pmu, &PMU::readySample, this, &PhasorPolarView::addSample);
}

void PhasorPolarView::addSample(const PMU::Sample &sample)
{
    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        auto [an, mg] = sample.toPolar(i);
        an = toDegrees(an);
        m_series[i]->replace(m_series[i]->count() - 1, {normalF(an), mg});
    }
}

constexpr qreal PhasorPolarView::toDegrees(qreal angle_rad) {
    return angle_rad * (180.0 / M_PI);
}

constexpr qint32 PhasorPolarView::normal(qint32 angle_deg) {
    return ((angle_deg % 360) + 360) % 360;
}

constexpr qreal PhasorPolarView::normalF(qreal angle_deg) {
    angle_deg = fmod(angle_deg, 360.0);
    return angle_deg + ((angle_deg < 0.0) * 360.0);
}

constexpr qint32 PhasorPolarView::toAngleOnAxis(qint32 angleActual_deg) {
    return normal(-angleActual_deg - 90);
}

constexpr qreal PhasorPolarView::toAngleOnAxisF(qreal angleActual_deg) {
    return normalF(-angleActual_deg - 90);
}

constexpr qint32 PhasorPolarView::toAngleActual(qint32 angleOnAxis_deg) {
    return normal(-angleOnAxis_deg + 90);
}

constexpr qreal PhasorPolarView::toAngleActualF(qreal angleOnAxis_deg) {
    return normalF(-angleOnAxis_deg + 90.0);
}
