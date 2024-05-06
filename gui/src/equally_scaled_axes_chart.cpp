#include "equally_scaled_axes_chart.h"

EquallyScaledAxesChart::EquallyScaledAxesChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(parent, wFlags)
{
}

void EquallyScaledAxesChart::resizeEvent(QGraphicsSceneResizeEvent *event)
{

    QChart::resizeEvent(event);

    auto p1 = mapToPosition(QPointF(0, 0));
    auto p2 = mapToPosition(QPointF(1, 1));
    auto ratio = std::abs(p2.x() - p1.x()) / std::abs(p2.y() - p1.y());

    auto s = event->newSize();
    s.setHeight(s.height() * ratio);

    resize(s);
}
