#ifndef PHASORPOLARVIEW_H
#define PHASORPOLARVIEW_H

#include "appcentralwidget.h"
#include "apptoolbar.h"
#include "phasor.h"

#include <QLineSeries>
#include <QWidget>
#include <QtCharts>

class PhasorPolarView : public QChartView {
    Q_OBJECT

public:
    PhasorPolarView(QWidget* parent = nullptr);

private:
    static QPointF toPointF(const Phasor::Value& value);
    static constexpr qint32 normal(qint32 angle);
    static constexpr qreal normalF(qreal angle);
    static constexpr qint32 toAngleOnAxis(qint32 angleActual);
    static constexpr qreal toAngleOnAxisF(qreal angleActual);
    static constexpr qint32 toAngleActual(qint32 angleOnAxis);
    static constexpr qreal toAngleActualF(qreal angleOnAxis);
};

#endif // PHASORPOLARVIEW_H
