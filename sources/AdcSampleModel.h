//
// Created by nadman on 11/15/23.
//

#ifndef ADCSAMPLEMODEL_H
#define ADCSAMPLEMODEL_H

#include <array>

#include <QList>
#include <QMetaType>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QPointF>
#include <QtNetwork>
#include <QtCharts>
#include <QValueAxis>

#include "SignalInfo.h"

// 6 signals + 1 timestamp + 1 delta
using AdcSampleVector = std::array<quint64, 8>;
Q_DECLARE_METATYPE(AdcSampleVector)

class AdcSampleModel: public QObject
{
Q_OBJECT

public:
    static constexpr qsizetype WindowSize = 100;

    explicit AdcSampleModel(QObject *parent = nullptr);

private:
    QTcpSocket *socket = nullptr;
    QList<quint64> valuesX;
    QList<quint64> valuesY;
    QMap<quint64, uint_fast16_t> countX;
    QMap<quint64, uint_fast16_t> countY;
    QMutex mutex;

    // Series points
    std::array<QList<QPointF>, 6> seriesPoints = {};

    void add(const AdcSampleVector &v);

public slots:
    void updateSeriesAndAxes(const std::array<QSplineSeries *, NumSignals> &series,
                             QtCharts::QValueAxis *axisX,
                             QtCharts::QValueAxis *axisY);

private slots:
    void read();
};

#endif //ADCSAMPLEMODEL_H
