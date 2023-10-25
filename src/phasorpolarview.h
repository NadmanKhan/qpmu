#ifndef PHASORPOLARVIEW_H
#define PHASORPOLARVIEW_H

#include "abstractphasorview.h"
#include "appdockwidget.h"

#include <QLineSeries>
#include <QWidget>
#include <QtCharts>
#include <QtMath>

class PhasorPolarView : public AbstractPhasorView {
    Q_OBJECT

public:
    PhasorPolarView(QWidget* parent = nullptr);

private:
    static constexpr qreal toDegrees(qreal angle_rad);
    static constexpr qint32 normal(qint32 angle_deg);
    static constexpr qreal normalF(qreal angle_deg);
    static constexpr qint32 toAngleOnAxis(qint32 angleActual_deg);
    static constexpr qreal toAngleOnAxisF(qreal angleActual_deg);
    static constexpr qint32 toAngleActual(qint32 angleOnAxis_deg);
    static constexpr qreal toAngleActualF(qreal angleOnAxis_deg);

private slots:
    void addSample(const PMU::Sample &sample);
};

#endif // PHASORPOLARVIEW_H
