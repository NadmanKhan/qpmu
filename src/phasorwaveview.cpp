#include "phasorwaveview.h"
#include <algorithm>

PhasorWaveView::PhasorWaveView(QWidget* parent)
    : AbstractPhasorView(new QChart(), parent) {

    auto chart = this->chart();

    m_axisX = new QValueAxis();
    m_axisX->setLabelsVisible(false);
    chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY = new QValueAxis();
    chart->addAxis(m_axisY, Qt::AlignLeft);

    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        auto s = new QSplineSeries(this);
        chart->addSeries(s);

        s->setName(PMU::names[i]);
        s->setPen(QPen(QBrush(PMU::colors[i]), 2.0));
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
        addSeriesToControl(s, PMU::types[i]);
        m_series[i] = s;
    }

    connect(pmu, &PMU::readySample, this, &PhasorWaveView::addSample);
}

void PhasorWaveView::addSample(const PMU::Sample &sample)
{
    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        const auto& point = sample.toRectangular(i);
        qreal x = point.first, y = point.second;
        m_series[i]->append({x, y});
        m_pointsWindow[i].append({x, y});
        if (m_pointsWindow[i].size() > MaxWindowLen) {
            m_pointsWindow[i].pop_front();
        }
    }
    qreal minX = std::numeric_limits<qreal>::max();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::min();
    qreal maxY = std::numeric_limits<qreal>::min();

    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        for (const auto& [x, y] : m_pointsWindow[i]) {
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
            minY = qMin(minY, y);
            maxY = qMax(maxY, y);
        }
    }
    m_axisX->setRange(minX, maxX);
    m_axisY->setRange(minY - 5, maxY + 5);
}
