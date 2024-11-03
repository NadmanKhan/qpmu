#include "qpmu/defs.h"
#include "qpmu/util.h"
#include "app.h"
#include "data_processor.h"
#include "src/settings_models.h"

#include <QDebug>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QMutexLocker>
#include <QLabel>
#include <QtGlobal>

#include <cctype>
#include <chrono>
#include <openc37118-1.0/c37118command.h>
#include <fcntl.h>
#include <qdir.h>
#include <unistd.h>

using namespace qpmu;

PhasorSender::PhasorSender()
{
    m_config2 = new CONFIG_Frame();
    m_config1 = new CONFIG_1_Frame();
    m_dataframe = new DATA_Frame(m_config2);
    m_header = new HEADER_Frame("PMU VERSAO 1.0 1");
    m_cmd = new CMD_Frame();

    // Create config packet
    constexpr U16 ID_CODE = 17;
    m_config2->IDCODE_set(ID_CODE);
    m_config1->IDCODE_set(ID_CODE);
    m_dataframe->IDCODE_set(ID_CODE);
    m_config2->DATA_RATE_set(50);
    m_config1->DATA_RATE_set(50);
    auto t = getDuration(SystemClock::now());
    m_config1->SOC_set(t.count() / 1000);
    m_config2->SOC_set(t.count() / 1000);
    m_dataframe->SOC_set(t.count() / 1000);
    m_config1->FRACSEC_set(t.count() % 1000);
    m_config2->FRACSEC_set(t.count() % 1000);
    m_dataframe->FRACSEC_set(t.count() % 1000);
    m_config1->TIME_BASE_set(TimeBase);
    m_config2->TIME_BASE_set(TimeBase);

    m_station = new PMU_Station("PMU 1", ID_CODE, true, true, true, false);

    for (USize i = 0; i < CountSignals; ++i) {
        m_station->PHASOR_add(NameOfSignal[i], 1, TypeOfSignal[i]);
    }

    m_station->FNOM_set(FN_50HZ);
    m_station->CFGCNT_set(1);
    m_station->STAT_set(2048);

    m_config2->PMUSTATION_ADD(m_station);
    m_config1->PMUSTATION_ADD(m_station);

    m_server = new QTcpServer(this);
    m_server->listen(QHostAddress::LocalHost, 4712);
}

void PhasorSender::attemptSend(const qpmu::Sample &sample, const qpmu::Estimation &estimation)
{
    int newState = 0;
    newState |= (Listening * bool(m_server->isListening()));
    if (!m_server->isListening()) {
        if (!m_server->listen(QHostAddress::LocalHost, 4712)) {
            qWarning("Failed to start listening on port 4712\n");
        }
    }

    if (m_server->waitForNewConnection(10)) {
        auto client = m_server->nextPendingConnection();
        if (client)
            m_clients.append(client);
    }
    newState |= (Connected * bool(m_clients.size() > 0));

    int deadClients = 0;
    for (int i = 0; i < m_clients.size(); ++i) {
        auto client = m_clients[i];
        bool isAlive = true;
        if (client->waitForReadyRead(10)) {
            auto n = client->read((char *)m_buffer, sizeof(m_buffer));

            if (n < 0) {
                qWarning("ERROR reading from socket, but continuing to run.\n");

            } else if (n == 0) {
                isAlive = false;

            } else /* if (n > 0) */ {
                if (m_buffer[0] == A_SYNC_AA) {
                    switch (m_buffer[1]) {
                    case A_SYNC_CMD:
                        qDebug() << "Command received\n";
                        m_cmd->unpack(m_buffer);
                        handleCommand(client);
                        break;
                    default:
                        qWarning("Unknown command received.\n");
                        break;
                    }
                }
            }
        }

        if (!isAlive) {
            qDebug() << "tcp server: client" << i << "is dead";
            std::swap(m_clients[i], m_clients[m_clients.size() - 1 - deadClients]);
            ++deadClients;
        }
    }

    if (m_sendDataFlag) {
        newState |= DataSending;
        updateData(sample, estimation);
        qDebug() << "tcp server: sending data:" << toString(sample).c_str()
                 << toString(estimation).c_str();
        char *dataBuffer = nullptr;
        unsigned short size = m_dataframe->pack((unsigned char **)&dataBuffer);
        for (int i = 0; i < m_clients.size() - deadClients; ++i) {
            auto client = m_clients[i];
            if (client && client->isOpen()) {
                auto n = client->write(dataBuffer, size);
                if (n < 0) {
                    qWarning("ERROR writing to socket, but continuing to run.\n");
                }
            }
        }
    } else {
        newState &= ~DataSending;
    }

    m_clients.resize(m_clients.size() - deadClients);

    m_state = newState;
}

void PhasorSender::handleCommand(QTcpSocket *client)
{
    int n;
    unsigned short size;
    unsigned char *buffer;

    switch (m_cmd->CMD_get()) {
    case 0x01: // Disable Data Output
        qDebug() << "Disable Data Output\n";
        m_sendDataFlag = false;
        break;

    case 0x02: // Enable Data Output
        qDebug() << "Enable Data Output\n";
        m_sendDataFlag = true;
        break;

    case 0x03: // Transmit Header Record Frame
        qDebug() << "Transmit Header Record Frame\n";
        size = m_header->pack(&buffer);
        n = client->write((char *)buffer, size);
        free(buffer);
        buffer = NULL;
        if (n < 0) {
            qWarning("ERROR writing to socket, but continuing to run.\n");
        }
        break;

    case 0x04: // Transmit Configuration #1 Record Frame
        qDebug() << "Transmit Configuration #1 Record Frame\n";
        size = m_config1->pack(&buffer);
        n = client->write((char *)buffer, size);
        free(buffer);
        buffer = NULL;
        if (n < 0) {
            qWarning("ERROR writing to socket, but continuing to run.\n");
        }
        break;

    case 0x05: // Transmit Configuration #2 Record Frame
        qDebug() << "Transmit Configuration #2 Record Frame\n";
        size = m_config2->pack(&buffer);
        n = client->write((char *)buffer, size);
        free(buffer);
        buffer = NULL;
        if (n < 0) {
            qWarning("ERROR writing to socket, but continuing to run.\n");
        }
        break;

    default:
        break;
    }
}

void PhasorSender::updateData(const qpmu::Sample &sample, const qpmu::Estimation &estimation)
{
    m_dataframe->SOC_set(sample.timestamp.count() / 1000);
    m_dataframe->FRACSEC_set(sample.timestamp.count() % 1000);
    for (USize i = 0; i < CountSignals; ++i) {
        m_station->PHASOR_VALUE_set(estimation.phasors[i], i);
    }
    m_station->FREQ_set(estimation.frequencies[0]);
    m_station->DFREQ_set(estimation.rocofs[0]);
}

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);
    m_sender = new PhasorSender();
    m_sender->moveToThread(this);
}

void DataProcessor::run()
{
    auto rpmsg_file_path = qgetenv("ADC_RPMSG_FILE");
    qDebug() << "ADC_RPMSG_FILE: " << rpmsg_file_path;
    auto fd = open(rpmsg_file_path, O_RDWR);
    if (fd < 0) {
        perror("Failed to open PRU RPMSG device");
        qFatal("Failed to open PRU RPMSG device\n");
    }

    RPMsg_Buffer msgBuf;
    Sample sample;

    constexpr USize TimeQueryPoint = 1000;
    USize counter = 0;
    auto lastTime = getDuration(SystemClock::now());
    auto lastPRUTimeNsec = 0;

    while (true) {
        if (write(fd, 0, 0) < 0) {
            qFatal("Failed to write to PRU RPMSG device\n");
        }

        auto nread = read(fd, (char *)&msgBuf, sizeof(msgBuf));
        qDebug() << "nread: " << nread;

        if (nread == sizeof(RPMsg_Buffer)) {
            if (counter == 0) {
                sample.timestamp = getDuration(SystemClock::now());
            } else {
                sample.timestamp = Duration(lastTime.count()
                                            + (msgBuf.timestampNsec - lastPRUTimeNsec) / 1000);
            }

            counter = (counter + 1) % TimeQueryPoint;
            lastTime = sample.timestamp;
            lastPRUTimeNsec = msgBuf.timestampNsec;

            for (USize i = 0; i < CountSignals; ++i) {
                sample.channels[i] = msgBuf.data[i];
            }

            qDebug() << "sample: " << toString(sample).c_str();
            m_estimator->updateEstimation(sample);

            auto senderOldState = m_sender->state();
            m_sender->attemptSend(sample, m_estimator->currentEstimation());
            auto senderNewState = m_sender->state();
            if (senderOldState != senderNewState) {
                emit phasorSenderStateChanged(senderNewState);
            }

            // if (readerNewState & SampleReader::DataReading) {
            //     for (USize i = 0; i < nread; ++i) {

            //         const auto &sample = m_sampleReadBuffer[i];
            //         for (USize j = 1; j < SampleStoreBufferSize; ++j) {
            //             m_sampleStoreBuffer[j - 1] = m_sampleStoreBuffer[j];
            //         }
            //         m_sampleStoreBuffer[SampleStoreBufferSize - 1] = sample;
            //         m_estimator->updateEstimation(sample);

            //         auto senderOldState = m_sender->state();
            //         m_sender->attemptSend(sample, m_estimator->currentEstimation());
            //         auto senderNewState = m_sender->state();
            //         if (senderOldState != senderNewState) {
            //             emit phasorSenderStateChanged(senderNewState);
            //         }
            //     }
            // }
            // if (readerOldState != readerNewState) {
            //     emit sampleReaderStateChanged(readerNewState);
            // }
        }
    }
}
