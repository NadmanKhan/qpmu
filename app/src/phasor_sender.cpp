#include "phasor_sender.h"
#include "qpmu/defs.h"
#include "qpmu/util.h"

#include <QTcpSocket>
#include <openc37118-1.0/c37118.h>

using namespace qpmu;

PhasorSender::PhasorSender()
{
    m_config2 = new CONFIG_Frame();
    m_config1 = new CONFIG_1_Frame();
    m_dataframe = new DATA_Frame(m_config2);
    m_header = new HEADER_Frame("PMU VERSAO 1.0 1");
    m_cmd = new CMD_Frame();

    // Create config packet
    constexpr uint16_t ID_CODE = 17;
    m_config2->IDCODE_set(ID_CODE);
    m_config1->IDCODE_set(ID_CODE);
    m_dataframe->IDCODE_set(ID_CODE);
    m_config2->DATA_RATE_set(50);
    m_config1->DATA_RATE_set(50);
    auto t = epochTime(SystemClock::now()).count();
    m_config1->SOC_set(t / 1000);
    m_config2->SOC_set(t / 1000);
    m_dataframe->SOC_set(t / 1000);
    m_config1->FRACSEC_set(t % 1000);
    m_config2->FRACSEC_set(t % 1000);
    m_dataframe->FRACSEC_set(t % 1000);
    m_config1->TIME_BASE_set(TimeDenom);
    m_config2->TIME_BASE_set(TimeDenom);

    m_station = new PMU_Station("PMU 1", ID_CODE, true, true, true, false);

    for (uint64_t i = 0; i < CountSignals; ++i) {
        m_station->PHASOR_add(NameOfSignal[i], 1,
                              TypeOfSignal[i] == VoltageSignal ? PHUNIT_Bit::VOLTAGE
                                                               : PHUNIT_Bit::CURRENT);
    }

    m_station->FNOM_set(FN_50HZ);
    m_station->CFGCNT_set(1);
    m_station->STAT_set(2048);

    m_config2->PMUSTATION_ADD(m_station);
    m_config1->PMUSTATION_ADD(m_station);

    m_server = new QTcpServer();
    m_server->moveToThread(this);

    m_sleepTime = int(1000.0 / m_config2->DATA_RATE_get());
}

void PhasorSender::run()
{
    m_state = 0;
    m_settings.load();
    m_server->setMaxPendingConnections(1); // Only one connection at a time
    auto listening = m_server->listen(QHostAddress(m_settings.socketConfig.host),
                                      m_settings.socketConfig.port);
    auto addr =
            QString("%1:%2").arg(m_settings.socketConfig.host).arg(m_settings.socketConfig.port);

    if (!listening) {
        qWarning() << "Failed to start listening on " << addr;
        qWarning() << "Error: " << m_server->errorString();
    } else {
        qDebug() << "* PMU with ID: " << m_config1->IDCODE_get();
        qDebug() << "* Listening on: " << addr;
        qDebug() << "* At data rate: " << m_config1->DATA_RATE_get() << "Hz";
        m_state |= Listening;
    }
    emit stateChanged(m_state);

    while (m_keepRunning) {
        int newState = 0;

        // Check if the server is listening
        newState |= (Listening * bool(m_server->isListening()));
        if (!m_server->isListening()) {
            if (!m_server->listen(m_server->serverAddress(), m_server->serverPort())) {
                qWarning("Failed to start listening on port 4712");
            }
        }

        // Check if there are new connections
        if (m_server->waitForNewConnection(100)) {
            auto client = m_server->nextPendingConnection();
            if (client)
                m_clients.append(client);
        }
        newState |= (Connected * bool(m_clients.size() > 0));

        // Handle clients
        int deadClients = 0;
        for (int i = 0; i < m_clients.size(); ++i) {
            auto client = m_clients[i];
            bool isAlive = true;
            if (client->waitForReadyRead(100)) {
                auto nread = client->read((char *)m_buffer, sizeof(m_buffer));

                if (nread < 0) {
                    qWarning("ERROR reading from socket, but continuing to run.");

                } else if (nread == 0) {
                    isAlive = false;

                } else /* if (n > 0) */ {
                    if (m_buffer[0] == A_SYNC_AA) {
                        switch (m_buffer[1]) {
                        case A_SYNC_CMD:
                            qDebug() << "PhasorSender: Command received";
                            m_cmd->unpack(m_buffer);
                            handleCommand(client);
                            break;
                        default:
                            qWarning("Unknown command received.");
                            break;
                        }
                    }
                }
            }

            if (!isAlive) {
                qDebug() << "PhasorSender: client" << i << "is dead";
                std::swap(m_clients[i], m_clients[m_clients.size() - 1 - deadClients]);
                ++deadClients;
            }
        }

        // Send data
        if (m_sendDataFlag) {
            newState |= DataSending;
            updateData(m_sample, m_estimation);
            // qDebug() << "PhasorSender: sending data:" << toString(m_sample).c_str()
            //          << toString(m_estimation).c_str();
            char *dataBuffer = nullptr;
            unsigned short size = m_dataframe->pack((unsigned char **)&dataBuffer);
            for (int i = 0; i < m_clients.size() - deadClients; ++i) {
                auto client = m_clients[i];
                if (client && client->isOpen()) {
                    auto n = client->write(dataBuffer, size);
                    if (n < 0) {
                        qWarning("ERROR writing to socket, but continuing to run.");
                    }
                }
            }
        } else {
            newState &= ~DataSending;
        }

        // Remove dead clients
        m_clients.resize(m_clients.size() - deadClients);

        // Update state
        if (newState != m_state) {
            emit stateChanged(newState);
            m_state = newState;
        }

        // Sleep to match the data rate
        msleep(m_sleepTime);
    }

    for (auto client : m_clients) {
        client->close();
    }
    m_server->close();
    emit stateChanged(m_server->isListening() ? Listening : 0);
}

void PhasorSender::handleCommand(QTcpSocket *client)
{
    switch (m_cmd->CMD_get()) {
    case 0x01: { // Disable Data Output
        qDebug() << "PhasorSender: Disable Data Output";
        m_sendDataFlag = false;
        break;
    }
    case 0x02: { // Enable Data Output
        qDebug() << "PhasorSender: Enable Data Output";
        m_sendDataFlag = true;
        break;
    }
    case 0x03: { // Transmit Header Record Frame
        qDebug() << "PhasorSender: Transmit Header Record Frame";

        uint8_t *buffer;
        auto size = m_header->pack(&buffer);

        auto nwrite = client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    case 0x04: { // Transmit Configuration #1 Record Frame
        qDebug() << "PhasorSender: Transmit Configuration #1 Record Frame";

        uint8_t *buffer;
        auto size = m_config1->pack(&buffer);

        auto nwrite = client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    case 0x05: { // Transmit Configuration #2 Record Frame
        qDebug() << "PhasorSender: Transmit Configuration #2 Record Frame";

        uint8_t *buffer;
        auto size = m_config2->pack(&buffer);

        auto nwrite = client->write((char *)buffer, size);
        std::free(buffer);

        if (nwrite < 0) {
            qWarning("ERROR writing to socket, but continuing to run.");
        }
        break;
    }
    }
}

void PhasorSender::updateData(const qpmu::Sample &sample, const qpmu::Estimation &estimation)
{
    QMutexLocker locker(&m_mutex);

    m_sample = sample;
    m_estimation = estimation;

    m_dataframe->SOC_set(m_sample.timestampUsec / 1000);
    m_dataframe->FRACSEC_set(m_sample.timestampUsec % 1000);

    for (uint64_t i = 0; i < CountSignals; ++i) {
        m_station->PHASOR_VALUE_set(m_estimation.phasors[i], i);
    }
    m_station->FREQ_set(m_estimation.frequencies[0]);
    m_station->DFREQ_set(m_estimation.rocofs[0]);
}

void PhasorSender::stopRunning()
{
    QMutexLocker locker(&m_mutex);
    m_keepRunning = false;
}
