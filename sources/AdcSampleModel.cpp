#include "AdcSampleModel.h"

AdcSampleModel::AdcSampleModel(QObject *parent)
    : QObject(parent)
{
    socket = new QTcpSocket(this);
    Q_ASSERT(connect(socket, &QTcpSocket::readyRead,
                     this, &AdcSampleModel::read));
    socket->connectToHost(QHostAddress::LocalHost, 12345);
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
    valuesX.append(v[6]);
    for (int i = 0; i < 6; ++i) {
        valuesY.append(v[i]);
    }
    ++countX[v[6]];
    for (int i = 0; i < 6; ++i) {
        ++countY[v[i]];
    }
    if (valuesX.size() > WindowSize) {
        auto x = valuesX.takeFirst();
        if (!--countX[x]) {
            countX.remove(x);
        }
        for (int i = 0; i < 6; ++i) {
            auto y = valuesY.takeFirst();
            if (!--countY[y]) {
                countY.remove(y);
            }
        }
    }
    emit dataReady(v, countX.firstKey(), countX.lastKey(),
                   countY.firstKey(), countY.lastKey());
}
