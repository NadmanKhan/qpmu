#include "rawdatareader.h"

RawDataReader::RawDataReader(const QString &program,
                             const QStringList &arguments,
                             QObject *parent)
    : QObject{parent}, m_process{this}
{
    m_process.start(program, arguments);

    connect(&m_process, &QProcess::readyReadStandardError,
            this, &RawDataReader::readStandardError);

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &RawDataReader::readStandardOutput);
}

void RawDataReader::readStandardError()
{
    std::cout << "Error from process " << m_process.program().toStdString() << ":\n"
             << m_process.readAllStandardOutput().toStdString() << "\n";
}

void RawDataReader::readStandardOutput()
{
    char kvPair[2][32];
    auto& key = kvPair[0];
    auto& value = kvPair[1];

    RawDataReader::ChannelReadings ch;
    quint64 ts;
    quint64 delta;

    int i = 0;
    bool chooseKV = 0;

    for (char c : m_process.readAllStandardOutput()) {

        switch (c) {

        case ' ':
            break;

        case '\n':
            m_results.append(Result{ch, ts, delta});
            ch.clear();
            break;

        case '=':
            i = 0;
            chooseKV = 1; // read value
            break;

        case ',':
            value[i] = '\0';

            if (key[0] == 'c') {
                ch.push_back(QString(value).toDouble());

            } else if (key[0] == 't') {
                ts = QString(value).toULongLong();

            } else {
                delta = QString(value).toULongLong();
            }

            i = 0;
            chooseKV = 0; // read key
            break;

        default:
            kvPair[chooseKV][i++] = c;
        }
    }

    sendData();
}

void RawDataReader::sendData()
{
    emit dataReady(std::move(m_results));
    m_results.clear();
}
