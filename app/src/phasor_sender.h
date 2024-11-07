#ifndef QPMU_APP_PHASOR_SENDER_H
#define QPMU_APP_PHASOR_SENDER_H

#include "qpmu/defs.h"

#include <openc37118-1.0/c37118.h>
#include <openc37118-1.0/c37118configuration.h>
#include <openc37118-1.0/c37118pmustation.h>
#include <openc37118-1.0/c37118data.h>
#include <openc37118-1.0/c37118header.h>
#include <openc37118-1.0/c37118command.h>

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpServer>
#include <sys/types.h>

class PhasorSender : public QThread
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
            m_server->deleteLater();
        }
        delete m_station;
        delete m_config2;
        delete m_config1;
        delete m_dataframe;
        delete m_header;
        std::free(m_cmd); // Because it is allocated with `malloc` in `CMD_Frame::unpack`
    }

    void run() override;

    static QString stateFlagName(StateFlag flag)
    {
        switch (flag) {
        case Listening:
            return QStringLiteral("Listening");
        case Connected:
            return QStringLiteral("Connected");
        case DataSending:
            return QStringLiteral("Data Sending");
        default:
            return QStringLiteral("Unknown");
        }
    }

    static QString stateString(int state)
    {
        return QStringLiteral("%1, %2, %3")
                .arg(bool(state & Listening) ? "Listening" : "")
                .arg(bool(state & Connected) ? "Connected" : "")
                .arg(bool(state & DataSending) ? "Data Sending" : "");
    }

    int state() const { return m_state; }
    QTcpServer *server() const { return m_server; }

    void handleCommand(QTcpSocket *client);
    
    void attemptSend();

    void updateData(const qpmu::Sample &sample, const qpmu::Estimation &estimation);

signals:
    void stateChanged(int state);

private:
    int m_state = 0;
    QTcpServer *m_server = nullptr;
    QVector<QTcpSocket *> m_clients = {};
    uint8_t m_buffer[10000] = {};

    QMutex m_mutex;
    qpmu::Sample m_sample = {};
    qpmu::Estimation m_estimation = {};

    PMU_Station *m_station = nullptr;
    CONFIG_Frame *m_config2 = nullptr;
    CONFIG_1_Frame *m_config1 = nullptr;
    DATA_Frame *m_dataframe = nullptr;
    HEADER_Frame *m_header = nullptr;
    CMD_Frame *m_cmd = nullptr;
    bool m_sendDataFlag = false;
};

#endif // QPMU_APP_PHASOR_SENDER_H