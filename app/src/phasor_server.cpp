#include "phasor_server.h"
#include "data_processor.h"
#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <QDateTime>
#include <QTcpSocket>

#include <openc37118-1.0/c37118.h>

using namespace qpmu;

PhasorServer::PhasorServer()
{
    m_settings.load();
    m_state = 0;

    {
        m_config2 = new CONFIG_Frame();
        m_config1 = new CONFIG_1_Frame();
        m_dataframe = new DATA_Frame(m_config2);
        m_header = new HEADER_Frame("PMU VERSAO 1.0 1");
        m_cmd = new CMD_Frame();

        constexpr uint16_t ID_CODE = 17;
        m_config2->IDCODE_set(ID_CODE);
        m_config1->IDCODE_set(ID_CODE);
        m_dataframe->IDCODE_set(ID_CODE);
        m_config2->DATA_RATE_set(50);
        m_config1->DATA_RATE_set(50);
        auto t = epochTime(SystemClock::now()).count();
        m_config1->SOC_set(t / TimeDenom);
        m_config2->SOC_set(t / TimeDenom);
        m_dataframe->SOC_set(t / TimeDenom);
        m_config1->FRACSEC_set(t % TimeDenom);
        m_config2->FRACSEC_set(t % TimeDenom);
        m_dataframe->FRACSEC_set(t % TimeDenom);
        m_config1->TIME_BASE_set(TimeDenom);
        m_config2->TIME_BASE_set(TimeDenom);

        m_station = new PMU_Station("PMU 1", ID_CODE, true, true, true, false);

        for (size_t i = 0; i < CountSignals; ++i) {
            m_station->PHASOR_add(NameOfSignal[i], 1,
                                  TypeOfSignal[i] == VoltageSignal ? PHUNIT_Bit::VOLTAGE
                                                                   : PHUNIT_Bit::CURRENT);
        }

        m_station->FNOM_set(FN_50HZ);
        m_station->CFGCNT_set(1);
        m_station->STAT_set(2048);

        m_config2->PMUSTATION_ADD(m_station);
        m_config1->PMUSTATION_ADD(m_station);
    }

    /// Client
    connect(this, &QTcpServer::newConnection, this, &PhasorServer::handleClientConnection);
    setMaxPendingConnections(1); // Only one connection at a time
    listen(QHostAddress(m_settings.socketConfig.host), m_settings.socketConfig.port);

    auto addrStr =
            QString("%1:%2").arg(m_settings.socketConfig.host).arg(m_settings.socketConfig.port);
    if (!isListening()) {
        qWarning() << "Failed to start listening on " << addrStr;
        qWarning() << "Error: " << errorString();
    } else {
        qDebug() << "* PMU with ID: " << m_config1->IDCODE_get();
        qDebug() << "* Listening on: " << addrStr;
        qDebug() << "* At data rate: " << m_config1->DATA_RATE_get() << "Hz";
        m_state |= Listening;
    }

    /// Timer
    m_timer = new QTimer(this);
    m_timer->setInterval(std::max(0, (int)(1000 / m_config2->DATA_RATE_get())));
    connect(m_timer, &QTimer::timeout, this, &PhasorServer::sendData);
    m_timer->start();
}

void PhasorServer::handleClientConnection()
{
    qDebug() << QDateTime::currentDateTime() << "PhasorServer: New connection";

    auto socket = nextPendingConnection();
    if (!socket)
        return;

    if (m_client) {
        disconnectClient();
    }

    m_client = socket;

    qInfo() << QDateTime::currentDateTime() << "PhasorServer: New connection from"
            << m_client->peerAddress().toString();

    connect(m_client, &QTcpSocket::readyRead, this, &PhasorServer::handleCommand);

    QMutexLocker locker(&m_mutex);
    m_state |= Connected;
}

void PhasorServer::disconnectClient()
{
    qInfo() << QDateTime::currentDateTime() << "PhasorServer: client disconnected";

    m_client->close();
    delete m_client;
    m_client = nullptr;

    QMutexLocker locker(&m_mutex);
    m_state &= ~Connected;
}

void PhasorServer::handleCommand()
{
    uint8_t buffer[1024];
    auto nread = m_client->read((char *)buffer, sizeof(buffer));

    if (nread < 0) {
        qWarning("ERROR reading from socket");
        return;
    }

    if (nread == 0) {
        disconnectClient();
        return;
    }

    if (buffer[0] == A_SYNC_AA) {
        if (buffer[1] == A_SYNC_CMD) {
            qInfo() << QDateTime::currentDateTime() << "PhasorServer: Command received";
            m_cmd->unpack(buffer);

        } else {
            qInfo() << QDateTime::currentDateTime() << "PhasorServer: Unknown command received";
            return;
        }
    }

    switch (m_cmd->CMD_get()) {
    case 0x01: { // Disable Data Output
        qInfo() << QDateTime::currentDateTime() << "PhasorServer: Disable Data Output";
        m_sendDataFlag = false;
        // if (m_state & DataSending) {
        //     disconnectClient();
        // }
        break;
    }
    case 0x02: { // Enable Data Output
        qInfo() << QDateTime::currentDateTime() << "PhasorServer: Enable Data Output";
        m_sendDataFlag = true;
        break;
    }
    case 0x03: { // Transmit Header Record Frame
        qInfo() << QDateTime::currentDateTime() << "PhasorServer: Transmit Header Record Frame";

        uint8_t *buffer = nullptr;
        auto size = m_header->pack(&buffer);

        auto nwrite = m_client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    case 0x04: { // Transmit Configuration #1 Record Frame
        qInfo() << QDateTime::currentDateTime()
                << "PhasorServer: Transmit Configuration #1 Record Frame";

        uint8_t *buffer = nullptr;
        auto size = m_config1->pack(&buffer);

        auto nwrite = m_client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    case 0x05: { // Transmit Configuration #2 Record Frame
        qInfo() << QDateTime::currentDateTime()
                << "PhasorServer: Transmit Configuration #2 Record Frame";

        uint8_t *buffer = nullptr;
        auto size = m_config2->pack(&buffer);

        auto nwrite = m_client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    }
}

void PhasorServer::sendData()
{
    if (!m_sendDataFlag || !m_client || !m_client->isOpen()) {
        QMutexLocker locker(&m_mutex);
        m_state &= ~DataSending;
        return;
    }

    { /// update data
        Sample sample;
        Estimation estimation;

        APP->dataProcessor()->getCurrent(sample, estimation);

        m_dataframe->SOC_set(sample.timestampUsec / TimeDenom);
        m_dataframe->FRACSEC_set(sample.timestampUsec % TimeDenom);

        for (size_t i = 0; i < CountSignals; ++i) {
            m_station->PHASOR_VALUE_set(estimation.phasors[i], i);
        }
        m_station->FREQ_set(estimation.frequencies[0]);
        m_station->DFREQ_set(estimation.rocofs[0]);
    }

    { /// send data
        uint8_t *buffer = nullptr;
        int64_t size = m_dataframe->pack(&buffer);

        int64_t nwrite = m_client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite == size) {
            // qInfo() << QDateTime::currentDateTime() << "PhasorServer: Data sent to client";
        } else if (nwrite < 0) {
            qWarning("ERROR writing to socket");
        }

        QMutexLocker locker(&m_mutex);
        m_state |= ((DataSending | Connected) * bool(nwrite == size));
    }
}