#include "AdcDataModel.h"

AdcDataModel::AdcDataModel(QObject *parent)
    : QObject(parent)
{
    const auto &app = qobject_cast<App *>(QApplication::instance());
    const auto &settings = app->settings();
    auto adcHostStr = settings->value("adc_address/host").toString();
    if (adcHostStr == "-") {
        qFatal("adc_address/host is not set");
    }
    auto adcPortStr = settings->value("adc_address/port", "-").toString();
    if (adcPortStr == "-") {
        qFatal("adc_address/port is not set");
    }
    auto adcSampleBufferSizeStr = settings->value("adc_data/buffer_size", "-").toString();
    if (adcSampleBufferSizeStr == "-") {
        qFatal("adc_data/buffer_size is not set");
    }
    bool ok = false;
    auto adcPort = adcPortStr.toUShort(&ok);
    if (!ok) {
        qFatal("adc_address/port is not a number");
    }
    auto adcHost = QHostAddress(adcHostStr);
    if (adcHost.isNull()) {
        qFatal("adc/host is not a valid IP address");
    }
    m_bufferSize = adcSampleBufferSizeStr.toUInt(&ok);
    if (!ok) {
        qFatal("adc_data/buffer_size is not a number");
    }

    socket = new QTcpSocket(this);

    Q_ASSERT(connect(socket, &QTcpSocket::readyRead,
                     this, &AdcDataModel::read));

    socket->connectToHost(adcHost, adcPort);
}

void AdcDataModel::read()
{
    auto data = socket->readAll();
    AdcSampleVector buffer = {};
    int index = 0;
    quint64 value = 0;
    char prv = 0;

    for (char ch: data) {
        if (ch == ',') {
            buffer[index] = value;
            value = 0;
            index = (index + 1) & 7;
            if (!index) {
                add(buffer);
            }
        }
        else if (isdigit(ch) && prv != 'h') {
            value = (value * 10) + (ch - '0');
        }
        prv = ch;
    }
}

void AdcDataModel::add(const AdcSampleVector &v)
{
    // append
    valuesX.append(v[6]);
    ++countX[v[6]];
    for (int i = 0; i < 6; ++i) {
        valuesY.append(v[i]);
        ++countY[v[i]];
        seriesPoints[i].append(QPointF(v[6], v[i]));
    }

    if (valuesX.size() > m_bufferSize) {
        // remove
        auto x = valuesX.takeFirst();
        if (--countX[x] == 0) {
            countX.remove(x);
        }
        for (int i = 0; i < 6; ++i) {
            auto y = valuesY.takeFirst();
            if (--countY[y] == 0) {
                countY.remove(y);
            }
            seriesPoints[i].removeFirst();
        }
    }
}

void AdcDataModel::getWaveformData(std::array<QList<QPointF>, 6> &seriesPoints,
                                     qreal &minX,
                                     qreal &maxX,
                                     qreal &minY,
                                     qreal &maxY)
{
    QMutexLocker locker(&mutex);
    minX = static_cast<qreal>(countX.firstKey());
    maxX = static_cast<qreal>(countX.lastKey());
    minY = static_cast<qreal>(countY.firstKey());
    maxY = static_cast<qreal>(countY.lastKey());
    seriesPoints = this->seriesPoints;
}

quint32 AdcDataModel::bufferSize() const
{
    return m_bufferSize;
}
