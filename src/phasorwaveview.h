#ifndef PHASORWAVEVIEW_H
#define PHASORWAVEVIEW_H

#include "abstractphasorview.h"
#include "appdockwidget.h"
#include "phasor.h"

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLineSeries>
#include <QSplineSeries>
#include <QWidget>
#include <QtCharts>

class PhasorWaveView : public AbstractPhasorView {
    Q_OBJECT

public:
    PhasorWaveView(QWidget* parent = nullptr);

private:
    QPointF toPointF(const Phasor::Value& value) const override;
};

#endif // PHASORWAVEVIEW_H
