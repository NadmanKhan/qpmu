#ifndef PHASOR_VIEW_H
#define PHASOR_VIEW_H

#include <QChartView>
#include <QObject>
#include <QWidget>
#include <QtCharts>
#include <QList>
#include <QString>
#include <QTimer>
#include <QLayout>
#include <QTableWidget>

#include <cmath>

#include "signal_info_model.h"
#include "worker.h"

class PhasorView : public QWidget
{
    Q_OBJECT
public:
    PhasorView(Worker *worker, QWidget *parent = nullptr);

private slots:
    void updateSeries();

private:
    QList<QLineSeries *> m_listLineSeries;
    QList<QVector<QPointF>> m_listLineSeriesPoints;
    QTableWidget *m_table;
    Worker *m_worker;
};

#endif // PHASOR_VIEW_H
