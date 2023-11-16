//
// Created by nadman on 11/15/23.
//

#ifndef ADCSAMPLEMODEL_H
#define ADCSAMPLEMODEL_H

#include <array>

#include <QMetaType>
#include <QMap>
#include <QObject>
#include <QtNetwork>

using AdcSampleVector = std::array<quint64, 8>; // 6 signals + 1 timestamp + 1 delta
Q_DECLARE_METATYPE(AdcSampleVector)

class AdcSampleModel: public QObject
{
Q_OBJECT

public:
    static constexpr uint_fast16_t WindowSize = 1000;

    explicit AdcSampleModel(QObject *parent = nullptr);

private:
    QTcpSocket *socket = nullptr;
    QList<quint64> valuesX = {};
    QList<quint64> valuesY = {};
    QMap<quint64, uint_fast16_t> countX = {};
    QMap<quint64, uint_fast16_t> countY = {};

    void add(const AdcSampleVector &v);

private slots:
    void read();

signals:
    void dataReady(AdcSampleVector vector,
                   qreal minX,
                   qreal maxX,
                   qreal minY,
                   qreal maxY);
};

#endif //ADCSAMPLEMODEL_H
