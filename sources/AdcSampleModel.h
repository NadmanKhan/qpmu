#ifndef ADCSAMPLEMODEL_H
#define ADCSAMPLEMODEL_H

#include <array>

#include <QList>
#include <QMetaType>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QPointF>
#include <QProcessEnvironment>
#include <QtNetwork>
#include <QtCharts>
#include <QValueAxis>

#include "SignalInfo.h"

// 6 signals + 1 timestamp + 1 delta
using AdcSampleVector = std::array<quint64, 8>;
Q_DECLARE_METATYPE(AdcSampleVector)

using PointsVector = std::array<QList<QPointF>, 6>;

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

public:

    void getWaveformData(std::array<QList<QPointF>, 6> &seriesPoints,
                         qreal &minX,
                         qreal &maxX,
                         qreal &minY,
                         qreal &maxY);

private slots:
    void read();
};

#endif //ADCSAMPLEMODEL_H
