#ifndef RAWDATAREADER_H
#define RAWDATAREADER_H

#include <QObject>
#include <QProcess>
#include <QVarLengthArray>
#include <QString>
#include <QColor>
#include <QDebug>
#include <QTextStream>
#include <QBuffer>

#include <utility>
#include <iostream>

constexpr qsizetype MaxNumChannels = 20;

class RawDataReader : public QObject
{
    Q_OBJECT
public:
    explicit RawDataReader(const QString &program,
                           const QStringList &arguments = {},
                           QObject *parent = nullptr);

    using ChannelReadings = QVarLengthArray<qreal, MaxNumChannels>;

    struct Result {
        ChannelReadings ch;
        quint64         ts;     /// timestamp
        quint64         delta;  /// delta of timestamp
    };

    using ResultList = QList<Result>;

private:

    ResultList m_results;
    QProcess m_process;

private slots:
    void readStandardError();
    void readStandardOutput();

public slots:
    void sendData();

signals:
    void dataReady(const ResultList& results);

};

#endif // RAWDATAREADER_H
