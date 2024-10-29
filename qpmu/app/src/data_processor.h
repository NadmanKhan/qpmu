#ifndef QPMU_APP_DATA_PROCESSOR_H
#define QPMU_APP_DATA_PROCESSOR_H

#include "qpmu/defs.h"
#include "qpmu/estimator.h"
#include "settings_models.h"
#include "app.h"

#include <QThread>
#include <QIODevice>
#include <QAbstractSocket>
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpServer>

#include <array>
#include <openc37118-1.0/c37118.h>
#include <openc37118-1.0/c37118configuration.h>
#include <openc37118-1.0/c37118pmustation.h>
#include <openc37118-1.0/c37118data.h>
#include <openc37118-1.0/c37118header.h>
#include <openc37118-1.0/c37118command.h>

#include <functional>

constexpr int ConnectionWaitTime = 300;
constexpr int ReadWaitTime = 100;

constexpr qpmu::USize SampleReadBufferSize = 1200;
using SampleReadBuffer = std::array<qpmu::Sample, SampleReadBufferSize>;

constexpr qpmu::USize SampleStoreBufferSize = 64;
using SampleStoreBuffer = std::array<qpmu::Sample, SampleStoreBufferSize>;

class SampleReader : public QObject
{
    Q_OBJECT
public:
    enum StateFlag {
        Enabled = 1 << 0,
        Connected = 1 << 1,
        DataReading = 1 << 2,
        DataValid = 1 << 3,
    };

    SampleReader() = default;
    SampleReader(const SamplerSettings &settings);
    virtual ~SampleReader()
    {
        if (m_device) {
            m_device->close();
        }
    }

    static QString stateString(int state)
    {
        return QStringLiteral("%1, %2, %3, %4")
                .arg(bool(state & Enabled) ? "Enabled" : "")
                .arg(bool(state & Connected) ? "Connected" : "")
                .arg(bool(state & DataReading) ? "DataReading" : "")
                .arg(bool(state & DataValid) ? "DataValid" : "");
    }

    SamplerSettings settings() const { return m_settings; }
    qpmu::USize attemptRead(SampleReadBuffer &outSample);
    int state() const { return m_state; }

private:
    SamplerSettings m_settings;
    QIODevice *m_device = nullptr;
    int m_state = 0;
    char m_line[1000];
    qpmu::RawSampleBatch m_batch = {};

    std::function<bool()> m_waitForConnected = nullptr;
    std::function<qpmu::USize(SampleReadBuffer &)> m_readSamples = nullptr;
};

class PhasorSender : public QObject
{
    Q_OBJECT
public:
    PhasorSender();
    ~PhasorSender()
    {
        if (m_server) {
            m_server->close();
        }
        delete m_station;
        delete m_config2;
        delete m_config1;
        delete m_dataframe;
        delete m_header;
        std::free(m_cmd); // Because it is allocated with `malloc` in `CMD_Frame::unpack`
    }

    QTcpServer *server() const { return m_server; }

    void attemptSend(const qpmu::Sample &sample, const qpmu::Estimation &estimation);
    void updateData(const qpmu::Sample &sample, const qpmu::Estimation &estimation);
    void handleCommand(QTcpSocket *client);

private:
    QTcpServer *m_server = nullptr;
    QVector<QTcpSocket *> m_clients = {};
    unsigned char m_buffer[10000] = {};

    PMU_Station *m_station = nullptr;
    CONFIG_Frame *m_config2 = nullptr;
    CONFIG_1_Frame *m_config1 = nullptr;
    DATA_Frame *m_dataframe = nullptr;
    HEADER_Frame *m_header = nullptr;
    CMD_Frame *m_cmd = nullptr;
    bool m_sendDataFlag = false;
};

class DataProcessor : public QThread
{
    Q_OBJECT

public:
    explicit DataProcessor();

    void run() override;

    int sampleReaderState()
    {
        QMutexLocker locker(&m_mutex);
        return m_reader->state();
    }
    const qpmu::Estimation &currentEstimation()
    {
        QMutexLocker locker(&m_mutex);
        return m_estimator->currentEstimation();
    }

    const SampleStoreBuffer &currentSampleBuffer()
    {
        QMutexLocker locker(&m_mutex);
        return m_sampleStoreBuffer;
    }

    void updateSampleReader();

signals:
    void sampleReaderStateChanged(int);

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;
    SampleStoreBuffer m_sampleStoreBuffer = {};
    SampleReadBuffer m_sampleReadBuffer = {};

    SampleReader *m_reader = nullptr;
    SampleReader *m_newReader = nullptr;
    PhasorSender *m_sender = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
