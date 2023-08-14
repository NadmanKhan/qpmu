#include "phasorwaveview.h"

PhasorWaveView::PhasorWaveView(QWidget* parent)
    : AbstractPhasorView(new QChart(), parent) {

    auto chart = this->chart();

    auto axisX = new QValueAxis();
    axisX->setTickCount(Phasor::capacity() + 1);
    axisX->setLabelsVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);

    auto axisY = new QValueAxis();
    axisY->setRange(-Phasor::maxAmplitude(), +Phasor::maxAmplitude());
    chart->addAxis(axisY, Qt::AlignLeft);

    auto rangeWidthX = Phasor::interval() * Phasor::capacity();

    for (int i = 0; i < (int)Phasor::phasors.size(); ++i) {
        const auto phasor = Phasor::phasors[i];
        auto values = phasor->values();
        auto minValue = values.front();

        auto series = new QLineSeries(this);
        chart->addSeries(series);

        series->setName(phasor->label);
        series->setPen(QPen(QBrush(phasor->color), 2.0));
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        for (auto v : values) series->append(toPointF(v));
        connect(
            phasor,
            &Phasor::newValueAdded,
            this,
            [this, series, axisX, rangeWidthX, minValue](const Phasor::Value& v) {
                auto maxX = v.timeStamp;
                auto minX = qMax(minValue.timeStamp, maxX - rangeWidthX);
                axisX->setRange(minX, maxX);
                series->append(toPointF(v));
            });

        addSeriesToControl(series, phasor->type);
    }
}

QPointF PhasorWaveView::toPointF(const Phasor::Value& value) const {
    return {(qreal)value.timeStamp, value.magnitude};
}
