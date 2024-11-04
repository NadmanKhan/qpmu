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

constexpr qpmu::USize SampleStoreSize = 64;
using SampleStore = std::array<qpmu::Sample, SampleStoreSize>;

class PhasorSender : public QObject
{
    Q_OBJECT
public:
    enum StateFlag {
        Listening = 1 << 0,
        Connected = 1 << 1,
        DataSending = 1 << 2,
    };

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

    static QString stateString(int state)
    {
        return QStringLiteral("%1, %2, %3")
                .arg(bool(state & Listening) ? "Listening" : "")
                .arg(bool(state & Connected) ? "Connected" : "")
                .arg(bool(state & DataSending) ? "DataSending" : "");
    }

    int state() const { return m_state; }
    QTcpServer *server() const { return m_server; }

    void attemptSend(const qpmu::Sample &sample, const qpmu::Estimation &estimation);
    void updateData(const qpmu::Sample &sample, const qpmu::Estimation &estimation);
    void handleCommand(QTcpSocket *client);

private:
    int m_state = 0;
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

    int phasorSenderState()
    {
        QMutexLocker locker(&m_mutex);
        return m_sender->state();
    }

    const qpmu::Estimation &currentEstimation()
    {
        QMutexLocker locker(&m_mutex);
        return m_estimator->currentEstimation();
    }

    const SampleStore &currentSampleStore()
    {
        QMutexLocker locker(&m_mutex);
        return m_sampleStore;
    }

signals:
    void phasorSenderStateChanged(int);

private:
    QMutex m_mutex;
    qpmu::PhasorEstimator *m_estimator = nullptr;
    SampleStore m_sampleStore = {};

    PhasorSender *m_sender = nullptr;
};

#endif // QPMU_APP_DATA_PROCESSOR_H
