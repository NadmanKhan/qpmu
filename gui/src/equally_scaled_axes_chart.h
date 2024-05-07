#ifndef EQUALLY_SCALED_AXES_CHART_H
#define EQUALLY_SCALED_AXES_CHART_H

#include <QtWidgets>
#include <QtCharts>
#include <QGraphicsSceneResizeEvent>
#include <QtGlobal>
#include <qnamespace.h>

class EquallyScaledAxesChart : public QChart
{
public:
    EquallyScaledAxesChart(Qt::Orientation stretchOriention, QGraphicsItem *parent = nullptr,
                           Qt::WindowFlags wFlags = Qt::WindowFlags());

protected:
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;

private:
    Qt::Orientation m_stretchOirentation;
};

#endif // EQUALLY_SCALED_AXES_CHART_H