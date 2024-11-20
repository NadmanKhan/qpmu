#ifndef QPMU_APP_PHASOR_SENDER_H
#define QPMU_APP_PHASOR_SENDER_H

#include "qpmu/defs.h"
#include "settings_models.h"

#include <openc37118-1.0/c37118.h>
#include <openc37118-1.0/c37118configuration.h>
#include <openc37118-1.0/c37118pmustation.h>
#include <openc37118-1.0/c37118data.h>
#include <openc37118-1.0/c37118header.h>
#include <openc37118-1.0/c37118command.h>

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpServer>
#include <QTcpSocket>

class PhasorServer : public QTcpServer
{
    Q_OBJECT
public:
    enum StateFlag {
        Listening = 1 << 0,
        Connected = 1 << 1,
        DataSending = 1 << 2,
    };

    PhasorServer();

    ~PhasorServer()
    {
        if (m_client)
            delete m_client;
        if (m_station)
            delete m_station;
        if (m_config2)
            delete m_config2;
        if (m_config1)
            delete m_config1;
        if (m_dataframe)
            delete m_dataframe;
        if (m_header)
            delete m_header;
        if (m_cmd)
            std::free(m_cmd); // Because it is allocated with `malloc` in `CMD_Frame::unpack`
    }

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
        return QStringLiteral("%1 %2, %3 %4, %5 %6")
                .arg(bool(state & Listening) ? "✅" : "❌")
                .arg("Listening")
                .arg(bool(state & Connected) ? "✅" : "❌")
                .arg("Connected")
                .arg(bool(state & DataSending) ? "✅" : "❌")
                .arg("Data Sending");
    }

    int connState()
    {
        QMutexLocker locker(&m_mutex);
        return m_state;
    }

private slots:
    void handleClientConnection();
    void disconnectClient();
    void handleCommand();
    void sendData();

private:
    QMutex m_mutex;

    NetworkSettings m_settings = {};
    QTcpSocket *m_client = nullptr;
    QTimer *m_timer = nullptr;
    bool m_sendDataFlag = false;
    int m_state = 0;

    PMU_Station *m_station = nullptr;
    CONFIG_Frame *m_config2 = nullptr;
    CONFIG_1_Frame *m_config1 = nullptr;
    DATA_Frame *m_dataframe = nullptr;
    HEADER_Frame *m_header = nullptr;
    CMD_Frame *m_cmd = nullptr;
};

#endif // QPMU_APP_PHASOR_SENDER_H