#ifndef EQUALLY_SCALED_AXES_CHART_H
#define EQUALLY_SCALED_AXES_CHART_H

#include <QtWidgets>
#include <QtCharts>
#include <QGraphicsSceneResizeEvent>

class EquallyScaledAxesChart : public QChart
{
public:
    EquallyScaledAxesChart(QGraphicsItem *parent = nullptr,
                          Qt::WindowFlags wFlags = Qt::WindowFlags());

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
};

#endif // EQUALLY_SCALED_AXES_CHART_H