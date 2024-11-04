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

#include <utility>
#include <cctype>
#include <chrono>
#include <openc37118-1.0/c37118command.h>
#include <fcntl.h>
#include <qdir.h>
#include <qstringliteral.h>
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
    auto t = epochTime(SystemClock::now()).count();
    m_config1->SOC_set(t / 1000);
    m_config2->SOC_set(t / 1000);
    m_dataframe->SOC_set(t / 1000);
    m_config1->FRACSEC_set(t % 1000);
    m_config2->FRACSEC_set(t % 1000);
    m_dataframe->FRACSEC_set(t % 1000);
    m_config1->TIME_BASE_set(TimeFracDenomUsec);
    m_config2->TIME_BASE_set(TimeFracDenomUsec);

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

    if (m_server->waitForNewConnection(100)) {
        auto client = m_server->nextPendingConnection();
        if (client)
            m_clients.append(client);
    }
    newState |= (Connected * bool(m_clients.size() > 0));

    int deadClients = 0;
    for (int i = 0; i < m_clients.size(); ++i) {
        auto client = m_clients[i];
        bool isAlive = true;
        if (client->waitForReadyRead(100)) {
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
    m_dataframe->SOC_set(sample.timestampUsec / 1000);
    m_dataframe->FRACSEC_set(sample.timestampUsec % 1000);
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

    if (APP->arguments().contains("--binary")) {
        qDebug() << "Reading data as binary";
        if (APP->arguments().contains("--pru")) {
            auto adc_stream = qgetenv("ADC_STREAM");
            qDebug() << "Reading raw (PRU-controlled ADC) data buffer (in binary) from RPMsg "
                        "device: "
                     << adc_stream;
            auto fd = open(adc_stream, O_RDWR);
            if (fd < 0) {
                perror("Failed to open PRU RPMsg device");
                qFatal("Failed to open PRU RPMsg device\n");
            }

            constexpr USize TimeQueryPoint = 1024;
            m_readSamples = [=](QString &error) -> USize {
                if (write(fd, 0, 0) < 0) {
                    error = QStringLiteral("Failed to write to PRU RPMsg device: %1")
                                    .arg(strerror(errno));
                    perror(error.toUtf8().constData());
                    return 0;
                }
                auto nread = read(fd, (char *)&m_rawReadState.streamBuf, sizeof(ADCStreamBuffer));
                if (nread == sizeof(ADCStreamBuffer)) {
                    Sample &s = m_sampleReadBuffer[0];
                    if (m_rawReadState.counter % TimeQueryPoint == 0) {
                        s.timestampUsec = epochTime(SystemClock::now()).count();
                    } else {
                        s.timestampUsec = m_rawReadState.lastTimeUsec
                                + (m_rawReadState.streamBuf.timestampNsec
                                   - m_rawReadState.lastBufTimeNsec)
                                        / 1000;
                    }
                    s.seq = m_rawReadState.counter;
                    s.timeDeltaUsec = s.timestampUsec - m_rawReadState.lastTimeUsec;
                    for (USize i = 0; i < CountSignals; ++i) {
                        s.channels[i] = m_rawReadState.streamBuf.data[i];
                    }
                    m_rawReadState.counter += 1;
                    m_rawReadState.lastTimeUsec = s.timestampUsec;
                    m_rawReadState.lastBufTimeNsec = m_rawReadState.streamBuf.timestampNsec;
                    return 1;
                } else {
                    error = QStringLiteral("Failed to read from PRU RPMsg device: %1")
                                    .arg(strerror(errno));
                    return 0;
                }
            };
        } else {
            qDebug() << "Reading processed samples (in binary) from stdin";
            m_readSamples = [=](QString &) -> USize {
                auto nread = fread(m_sampleReadBuffer.data(), sizeof(Sample),
                                   std::tuple_size<SampleReadBuffer>(), stdin);
                return nread;
            };
        }
    } else {
        qFatal("Not implemented\n");
    }
}

void DataProcessor::run()
{
    while (true) {

        QString error;
        auto nread = m_readSamples(error);
        if (!error.isEmpty()) {
            qWarning() << error;
            continue;
        }

        for (USize i = 0; i < nread; ++i) {
            const auto &sample = m_sampleReadBuffer[i];
            for (USize j = 1; j < m_sampleStore.size(); ++j) {
                m_sampleStore[j - 1] = m_sampleStore[j];
            }
            m_sampleStore.back() = sample;

            m_estimator->updateEstimation(sample);

            auto senderOldState = m_sender->state();
            m_sender->attemptSend(sample, m_estimator->currentEstimation());
            auto senderNewState = m_sender->state();

            if (senderOldState != senderNewState) {
                emit phasorSenderStateChanged(senderNewState);
            }
        }
    }
}
