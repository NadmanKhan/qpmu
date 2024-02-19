#include "AdcDataModel.h"

AdcDataModel::AdcDataModel(QObject *parent)
    : QObject(parent)
{

    // Retrieve and validate settings values from the App instance
    // -------------------------------------------------------------

    const auto &app = qobject_cast<App *>(QApplication::instance());
    const auto &settings = app->settings();

    if (!settings->contains("adc/type")) {
        qFatal("adc/type is not set");
    }
    if (!settings->contains("adc/buffer_length")) {
        qFatal("adc/buffer_length is not set");
    }

    auto adcType = settings->value("adc/type");
    m_bufferLength = settings->value("adc/buffer_length").toUInt();

    if (adcType == QStringLiteral("socket")) {
        ioDevice = new QTcpSocket(this);
        Q_ASSERT(connect(ioDevice, &QTcpSocket::readyRead,
                         this, &AdcDataModel::read));

        if (!settings->contains("adc/socket_host")) {
            qFatal("adc/socket_host is not set");
        }
        if (!settings->contains("adc/socket_port")) {
            qFatal("adc/socket_port is not set");
        }

        auto host = settings->value("adc/socket_host").toString();
        auto port = settings->value("adc/socket_port").toUInt();
        qobject_cast<QTcpSocket *>(ioDevice)->connectToHost(host, port);

    }
    else if (adcType == QStringLiteral("file")) {
        qDebug() << "Opening file: " << settings->value("adc/file_path").toString();
        ioDevice = new QFile(settings->value("adc/file_path").toString(), this);
        if (!ioDevice->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qFatal("Cannot open file");
        }
        // QFile does not emit readyRead signal
        read();
    }
    else if (adcType == QStringLiteral("program")) {
        auto process = new QProcess(this);
        auto programPath = settings->value("adc/program_path").toString();
        auto programArgs = settings->value("adc/program_args").toStringList();
        if (programArgs.last().isEmpty()) {
            programArgs.removeLast();
        }
        Q_ASSERT(connect(process, &QProcess::readyRead,
                         this, &AdcDataModel::read));
        ioDevice = process;
        connect(process, &QProcess::readyReadStandardError, [process]()
        {
            qFatal("Program error: %s", process->readAllStandardError().data());
        });
        qDebug() << "Starting program: " << programPath << programArgs;
        process->start(programPath, programArgs);
        process->waitForStarted(-1);
        qDebug() << "Program started: " << programPath << programArgs;

    }
    else {
        qFatal("adc/type is not valid");
    }
}

/// This implmentation for parsing the ADC data is made to be as fast as
/// possible because it does the minimum required to parse. The logic is ad-hoc.
/// Each sample is in the form:
/// "ch0={value},ch1={value},ch2={value},ch3={value},ch4={value},ch5={value},ts={value},delta={value},\n"
/// where {value} is an unsigned decimal integer.
void AdcDataModel::read()
{
    QByteArray bytes = ioDevice->readAll();
    AdcSampleVector vector = {}; // current vector
    quint64 value = 0;  // current value
    char prv = 0;  // previous byte
    int index = 0;  // index of the current value in vector

    for (char ch: bytes) {
        if (ch == ',') {
            // end of a value. store it and reset value.
            vector[index] = value;
            value = 0;
            index = (index + 1) & 7; // % 8
            if (index == 0) {
                // end of a line
                addSample(vector);
            }
        }
        else if (isdigit(ch) && prv != 'h') {
            // if we found a digit and the previous byte is not 'h' of "ch"
            // (meaning it's not a channel index), then append the digit to the
            // value.
            value = (value * 10) + (ch - '0');
        }
        prv = ch;
    }
}

void AdcDataModel::addSample(const AdcSampleVector &v)
{
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

    if (valuesX.size() > m_bufferLength) {
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

    if (countX.empty()) {
        minX = 0;
        maxX = 100;
        minY = 0;
        maxY = 100;
    }
    else {
        minX = static_cast<qreal>(countX.firstKey());
        maxX = static_cast<qreal>(countX.lastKey());
        minY = static_cast<qreal>(countY.firstKey());
        maxY = static_cast<qreal>(countY.lastKey());
    }

    seriesPoints = this->seriesPoints;
}

quint32 AdcDataModel::bufferLength() const
{
    return m_bufferLength;
}
