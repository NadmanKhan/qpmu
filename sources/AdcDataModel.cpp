#include "AdcDataModel.h"

AdcDataModel::AdcDataModel(QObject *parent)
    : QObject(parent)
{

    // Retrieve and validate settings values from the App instance
    // -------------------------------------------------------------

    const auto& app      = qobject_cast<App*>(QApplication::instance());
    const auto &settings = app->settings();

    auto adcHostStr = settings->value("adc_address/host", "-").toString();
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

    // Initialize the socket and connect to it
    // -------------------------------------------------------------

    socket = new QTcpSocket(this);

    Q_ASSERT(connect(socket, &QTcpSocket::readyRead,
                     this, &AdcDataModel::read));

    socket->connectToHost(adcHost, adcPort);
}

/// This implmentation for parsing the ADC data is made to be as fast as
/// possible because it does the minimum required to parse. The logic is ad-hoc.
/// Each sample is in the form:
/// "ch0={value},ch1={value},ch2={value},ch3={value},ch4={value},ch5={value},ts={value},delta={value},\n"
/// where {value} is an unsigned decimal integer.
void AdcDataModel::read() {
    QByteArray bytes       = socket->readAll();
    AdcSampleVector vector = {}; // current vector
    quint64 value          = 0;  // current value
    char prv               = 0;  // previous byte
    int index              = 0;  // index of the current value in vector

    for (char ch : bytes) {
        if (ch == ',') {
            // end of a value. store it and reset value.
            vector[index] = value;
            value         = 0;
            index         = (index + 1) & 7; // % 8
            if (index == 0) {
                // end of a line
                add_sample(vector);
            }
        } else if (isdigit(ch) && prv != 'h') {
            // if we found a digit and the previous byte is not 'h' of "ch"
            // (meaning it's not a channel index), then append the digit to the
            // value.
            value = (value * 10) + (ch - '0');
        }
        prv = ch;
    }
}

void AdcDataModel::add_sample(const AdcSampleVector& v) {
    // Append
    // -------------------------------------------------------------

    valuesX.append(v[6]); // timestamp
    ++countX[v[6]];       // access the frequency of timestamp and increment

    // Do the same for the signal values
    for (int i = 0; i < 6; ++i) {
        valuesY.append(v[i]);
        ++countY[v[i]];
        seriesPoints[i].append(QPointF(v[6], v[i]));
    }

    if (valuesX.size() > m_bufferSize) {
        // Remove first
        // -------------------------------------------------------------

        auto x = valuesX.takeFirst(); // pop the first timestamp
        if (--countX[x] == 0) {
            // if the frequency of timestamp is 0, also remove it from the map
            countX.remove(x);
        }

        // Do the same for the signal values
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

    // TODO: use std::minmax_element instead of QMap

    // Because QMap is a red-black tree, the first and last keys are the least
    // and greatest keys respectively.
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
