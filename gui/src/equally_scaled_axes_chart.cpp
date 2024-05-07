#include "equally_scaled_axes_chart.h"
#include <qnamespace.h>

EquallyScaledAxesChart::EquallyScaledAxesChart(Qt::Orientation stretchOriention,
                                               QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(parent, wFlags), m_stretchOirentation(stretchOriention)
{
}

void EquallyScaledAxesChart::resizeEvent(QGraphicsSceneResizeEvent *event)
{

    QChart::resizeEvent(event);

    auto p1 = mapToPosition(QPointF(0, 0));
    auto p2 = mapToPosition(QPointF(1, 1));
    auto w2h = std::abs(p2.x() - p1.x()) / std::abs(p2.y() - p1.y());

    auto s = event->newSize();

    if (m_stretchOirentation == Qt::Horizontal) {
        s.setWidth(s.width() / w2h);
    } else {
        s.setHeight(s.height() * w2h);
    }

    resize(s);
}
