#ifndef ABSTRACTPHASORVIEW_H
#define ABSTRACTPHASORVIEW_H

#include "appdockwidget.h"
#include "apptoolbar.h"
#include "pmu.h"

#include <QPair>
#include <QList>
#include <QVBoxLayout>
#include <QtCharts>
#include <QXYSeries>
#include <QVarLengthArray>

class AbstractPhasorView : public QChartView {
    Q_OBJECT
public:
    AbstractPhasorView(QChart* chart, QWidget* parent = nullptr);

    ~AbstractPhasorView();

    static constexpr qsizetype MaxWindowLen = 50;

protected:
    void addSeriesToControl(QXYSeries* series, PMU::PhasorType phasorType);
    AppToolBar* tb;
    AppDockWidget* dw;
    PMU* pmu;
    QVarLengthArray<QXYSeries*, PMU::NumChannels> m_series;
    QVarLengthArray<QList<QPointF>, PMU::NumChannels> m_pointsWindow;

private:
    QWidget* m_controlWidget;
    QVBoxLayout* m_controlLayout;
    QList<QPair<QCheckBox*, PMU::PhasorType>> m_controlCheckBoxes;

    // QWidget interface
private:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
};

#endif // ABSTRACTPHASORVIEW_H
