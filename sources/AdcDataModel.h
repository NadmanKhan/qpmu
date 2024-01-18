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

/**
 * @brief The AdcSampleVector struct containing
 * 6 signals + 1 timestamp + 1 timestamp delta.
 * Each signal value is 12 bits long, and the
 * timestamp is the UNIX timestamp in nanoseconds.
 */
using AdcSampleVector = std::array<quint64, 8>;
Q_DECLARE_METATYPE(
  AdcSampleVector) // required since AdcSampleVector is not a QObject

class AdcDataModel: public QObject
{
Q_OBJECT
public:
    explicit AdcDataModel(QObject *parent = nullptr);

private:
    /**
     * @brief The socket for receiving data from the ADC.
     */
    QTcpSocket* socket = nullptr;

    /**
     * @brief Size of the buffer for storing ADC data.
     * Retrieved from the settings.
     */
    quint32 m_bufferSize;

    /**
     * @brief Sequentially stored timestamp values in buffer, 1 per ADC sample.
     */
    QList<quint64> valuesX;

    /**
     * @brief Sequentially stored signal values in buffer, 6 per ADC sample.
     */
    QList<quint64> valuesY;

    /**
     * @brief RB-tree for efficiently storing and querying the least and
     * greatest timestamp currently in buffer. Superfluous
     * because timestamps are monotonically increasing, so we could
     * have just used the first and last timestamps in the buffer instead.
     */
    QMap<quint64, quint64> countX;

    /**
     * @brief RB-tree for efficiently storing and querying the least and
     * greatest signal value currently in buffer.
     */
    QMap<quint64, quint64> countY;

    /**
     * @brief The points to be plotted on the graph.
     */
    std::array<QList<QPointF>, 6> seriesPoints = {};

    /**
     * @brief Mutex for synchronizing access.
     */
    QMutex mutex;

    /**
     * @brief Add a single ADC sample to the buffer.
     * @param v The ADC sample vector.
     */
    void add_sample(const AdcSampleVector& v);

public:
    /**
     * @brief Public method for retrieving the data to be plotted on the graph.
     * @param seriesPoints [out] The points to be plotted on the graph.
     * @param minX [out] The least timestamp in the buffer.
     * @param maxX [out] The greatest timestamp in the buffer.
     * @param minY [out] The least signal value in the buffer.
     * @param maxY [out] The greatest signal value in the buffer.
     */
    void getWaveformData(std::array<QList<QPointF>, 6> &seriesPoints,
                         qreal &minX,
                         qreal &maxX,
                         qreal &minY,
                         qreal &maxY);

    /**
     * @brief Public method for retrieving the size of the buffer.
     * @return The size of the buffer.
     */
    [[nodiscard]] quint32 bufferSize() const;

private slots:

    /**
     * @brief Slot for handling the socket's `readyRead()` signal.
     */
    void read();
};

#endif //ADCDATAMODEL_H
