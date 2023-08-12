#ifndef PHASORWAVEVIEW_H
#define PHASORWAVEVIEW_H

#include "appcentralwidget.h"
#include "phasor.h"
#include "apptoolbar.h"

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLineSeries>
#include <QSplineSeries>
#include <QWidget>
#include <QtCharts>

class PhasorWaveView : public QChartView {
    Q_OBJECT

public:
    PhasorWaveView(QWidget* parent = nullptr);

signals:
    void visibilityChanged(bool visible);

private:
    static QPointF toPoint(const Phasor::Value& value);
};

#endif // PHASORWAVEVIEW_H
