#ifndef PHASORWAVEVIEW_H
#define PHASORWAVEVIEW_H

#include "abstractphasorview.h"
#include "appdockwidget.h"

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLineSeries>
#include <QSplineSeries>
#include <QWidget>
#include <QtCharts>

#include <iostream>
using std::cout;

class PhasorWaveView : public AbstractPhasorView {
    Q_OBJECT

public:
    PhasorWaveView(QWidget* parent = nullptr);

private:
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;

private slots:
    void addSample(const PMU::Sample &sample);
};

#endif // PHASORWAVEVIEW_H
