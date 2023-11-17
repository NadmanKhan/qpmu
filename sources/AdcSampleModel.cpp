#include "AdcSampleModel.h"

AdcSampleModel::AdcSampleModel(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);
    Q_ASSERT(connect(socket, &QTcpSocket::readyRead,
                     this, &AdcSampleModel::read));
    auto env = QProcessEnvironment::systemEnvironment();
    auto host = env.value("PMU_ADC_HOST", "-");
    auto port = env.value("PMU_ADC_PORT", "-");
    if (host == "-" || port == "-") {
        qFatal("PMU_ADC_HOST or PMU_ADC_PORT not set");
    }
    socket->connectToHost(host, port.toUShort());
}

void AdcSampleModel::read()
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

void AdcSampleModel::add(const AdcSampleVector &v)
{
    // append
    valuesX.append(v[6]);
    ++countX[v[6]];
    for (int i = 0; i < 6; ++i) {
        valuesY.append(v[i]);
        ++countY[v[i]];
        seriesPoints[i].append(QPointF(v[6], v[i]));
    }

    if (valuesX.size() > WindowSize) {
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

void AdcSampleModel::getWaveformData(std::array<QList<QPointF>, 6> &seriesPoints,
                                     qreal &minX,
                                     qreal &maxX,
                                     qreal &minY,
                                     qreal &maxY)
{
    QMutexLocker locker(&mutex);
    minX = countX.firstKey();
    maxX = countX.lastKey();
    minY = countY.firstKey();
    maxY = countY.lastKey();
    seriesPoints = this->seriesPoints;
}
