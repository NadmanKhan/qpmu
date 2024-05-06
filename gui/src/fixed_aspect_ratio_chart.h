#ifndef FIXED_ASPECT_RATIO_CHART_H
#define FIXED_ASPECT_RATIO_CHART_H

#include <QtWidgets>
#include <QtCharts>
#include <QGraphicsSceneResizeEvent>

class FixedAspectRatioChart : public QChart
{
public:
    FixedAspectRatioChart(QGraphicsItem *parent = nullptr,
                          Qt::WindowFlags wFlags = Qt::WindowFlags());

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
};

#endif // FIXED_ASPECT_RATIO_CHART_H