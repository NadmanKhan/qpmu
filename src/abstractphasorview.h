#ifndef ABSTRACTPHASORVIEW_H
#define ABSTRACTPHASORVIEW_H

#include "appdockwidget.h"
#include "apptoolbar.h"
#include "phasor.h"

#include <QPair>
#include <QList>
#include <QVBoxLayout>
#include <QtCharts>

class AbstractPhasorView : public QChartView {
    Q_OBJECT
public:
    AbstractPhasorView(QChart* chart, QWidget* parent = nullptr);

    ~AbstractPhasorView();

protected:
    virtual QPointF toPointF(const Phasor::Value& value) const = 0;
    void addSeriesToControl(QXYSeries* series, Phasor::Type phasorType);

private:
    QWidget* m_controlWidget;
    QVBoxLayout* m_controlLayout;
    QList<QPair<QCheckBox*, Phasor::Type>> m_controlCheckBoxes;

    AppToolBar* tb;
    AppDockWidget* dw;

    // QWidget interface
private:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
};

#endif // ABSTRACTPHASORVIEW_H
