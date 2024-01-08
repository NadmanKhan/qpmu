#ifndef ADCDATAMODEL_H
#define ADCDATAMODEL_H

#include <array>

#include <QList>
#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QPointF>
#include <QSettings>
#include <QtNetwork>
#include <QValueAxis>
#include <QHostAddress>

#include "App.h"
#include "SignalInfo.h"

// 6 signals + 1 timestamp + 1 delta
using AdcSampleVector = std::array<quint64, 8>;
Q_DECLARE_METATYPE(AdcSampleVector)

using PointsVector = std::array<QList<QPointF>, 6>;

class AdcDataModel: public QObject
{
Q_OBJECT
public:
    explicit AdcDataModel(QObject *parent = nullptr);

private:
    QTcpSocket *socket = nullptr;
    quint32 m_bufferSize;
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

    [[nodiscard]] quint32 bufferSize() const;

private slots:
    void read();
};

#endif //ADCDATAMODEL_H
