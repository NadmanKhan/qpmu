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
#include <netinet/in.h>
#include <openc37118-1.0/c37118command.h>

using namespace qpmu;

SampleReader::SampleReader(const SamplerSettings &settings) : m_settings(settings)
{
    if (m_settings.connection == SamplerSettings::Socket) {
        QAbstractSocket *socket;
        if (m_settings.socketConfig.socketType == SamplerSettings::TcpSocket)
            socket = new QTcpSocket(this);
        else
            socket = new QUdpSocket(this);
        m_device = socket;
        m_waitForConnected = [=] {
            return socket->state() >= QAbstractSocket::ConnectedState
                    || socket->waitForConnected(10);
        };
        socket->connectToHost(m_settings.socketConfig.host, m_settings.socketConfig.port);
    } else if (m_settings.connection == SamplerSettings::Process) {
        auto process = new QProcess(this);
        m_device = process;
        process->setProgram(m_settings.processConfig.prog);
        process->setArguments(m_settings.processConfig.args);
        m_waitForConnected = [=] {
            return process->state() >= QProcess::Running || process->waitForStarted(10);
        };
        process->start(QProcess::ReadOnly);
    }

    m_readSamples = [](SampleReadBuffer &) -> qpmu::USize { return 0; };

    if (settings.connection != SamplerSettings::None) {
        if (qgetenv("BATCHED_SAMPLES").toLower() == "true") {
            /// Read a batch of samples.
            /// Only works with binary data.
            if (settings.isDataBinary) {
                m_readSamples = [this](SampleReadBuffer &outSamples) -> qpmu::USize {
                    auto bytesRead = m_device->read((char *)&m_batch, sizeof(RawSampleBatch));
                    // qDebug() << "bytesRead: " << bytesRead;
                    // qDebug() << "sizeof(RawSampleBatch): " << sizeof(RawSampleBatch);
                    // qDebug() << "batchNo: " << m_batch.batchNo;
                    // qDebug() << "firstSampleTimeUs: " << m_batch.firstSampleTimeUs;
                    // qDebug() << "lastSampleTimeUs: " << m_batch.lastSampleTimeUs;
                    // qDebug() << "timeSinceLastBatchUs: " << m_batch.timeSinceLastBatchUs;
                    if (bytesRead == sizeof(RawSampleBatch)) {
                        auto timeWindow = m_batch.lastSampleTimeUs - m_batch.firstSampleTimeUs;
                        auto timeDelta = timeWindow / (CountRawSamplesPerBatch - 1);
                        for (USize i = 0; i < CountRawSamplesPerBatch; ++i) {
                            auto &rawSample = m_batch.samples[i];
                            Sample &sample = outSamples[i];
                            sample.seqNo = (m_batch.batchNo - 1) * CountRawSamplesPerBatch
                                    + rawSample.sampleNo;
                            sample.timestampUs = m_batch.firstSampleTimeUs + i * timeDelta;
                            sample.timeDeltaUs = timeDelta;
                            for (USize j = 0; j < CountSignals; ++j) {
                                sample.channels[j] = rawSample.data[j];
                            }
                            // qDebug() << "Sample " << i << ": " << toString(sample).c_str();
                        }
                        return CountRawSamplesPerBatch;
                    }
                    return 0;
                };
            } else {
                qWarning("Batched samples only work with binary data");
            }
        } else {
            /// Read just one sample at a time.
            if (settings.isDataBinary) {
                m_readSamples = [this](SampleReadBuffer &outSamples) -> qpmu::USize {
                    auto bytesRead = m_device->read((char *)outSamples.data(), sizeof(Sample));
                    if (bytesRead != sizeof(Sample)) {
                        qWarning("Failed to read a sample");
                        qDebug() << "bytesRead: " << bytesRead;
                        qDebug() << "sizeof(Sample): " << sizeof(Sample);
                    }
                    return (qpmu::USize)(bytesRead == sizeof(Sample));
                };
            } else {
                m_readSamples = [this](SampleReadBuffer &outSamples) -> qpmu::USize {
                    USize ok = 1;
                    if (m_device->readLine(m_line, sizeof(m_line)) > 0) {
                        std::string error;
                        outSamples[0] = parseSample(m_line, &error);
                        if (!error.empty()) {
                            ok = 0;
                            qDebug() << "Error parsing sample: " << error.c_str();
                            qDebug() << "Sample line:   " << m_line;
                            qDebug() << "Parsed sample: " << toString(outSamples[0]).c_str();
                        }
                    }
                    return ok;
                };
            }
        }
    }
}

qpmu::USize SampleReader::attemptRead(SampleReadBuffer &outSamples)
{
    int newState = 0;
    /// Check if enabled
    newState |= (Enabled * bool(m_settings.connection != SamplerSettings::None));
    /// Check if connected
    newState |= (Connected * bool((newState & Enabled) && m_waitForConnected()));
    /// Check if reading
    newState |= (DataReading
                 * bool((newState & Connected) && m_device->waitForReadyRead(ReadWaitTime)));
    /// Check if valid
    USize nread = 0;
    newState |= (DataValid * bool((newState & DataReading) && (nread = m_readSamples(outSamples))));

    m_state = newState;

    return nread;
}

PhasorSender::PhasorSender()
{
    m_config2 = new CONFIG_Frame();
    m_config1 = new CONFIG_1_Frame();
    m_dataframe = new DATA_Frame(m_config2);
    m_header = new HEADER_Frame("PMU VERSAO 1.0 LSEE");
    m_cmd = new CMD_Frame();

    U64 current_time = std::chrono::duration_cast<std::chrono::microseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

    // Create config packet
    constexpr U64 TIME_BASE = 1'000'000;
    constexpr U16 ID_CODE = 17;
    m_config2->IDCODE_set(ID_CODE);
    m_config2->SOC_set(current_time / TIME_BASE);
    m_config2->FRACSEC_set(current_time % TIME_BASE);
    m_config2->TIME_BASE_set(TIME_BASE);
    m_config2->DATA_RATE_set(100);

    m_config1->IDCODE_set(ID_CODE);
    m_config1->SOC_set(current_time / TIME_BASE);
    m_config1->FRACSEC_set(current_time % TIME_BASE);
    m_config1->TIME_BASE_set(TIME_BASE);
    m_config1->DATA_RATE_set(100);

    m_dataframe->IDCODE_set(ID_CODE);
    m_dataframe->SOC_set(current_time / TIME_BASE);
    m_dataframe->FRACSEC_set(current_time % TIME_BASE);

    m_station = new PMU_Station("PMU LSEE", ID_CODE, true, true, true, false);

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
    if (!m_server->isListening()) {
        qDebug() << "tcp server: not listening";
        return;
    }
    if (m_server->waitForNewConnection(10)) {
        qDebug() << "tcp server: new connection";
        auto client = m_server->nextPendingConnection();
        if (client)
            m_clients.append(client);
    }
    // qDebug() << "tcp server: count clients:" << m_clients.size();

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
    }

    m_clients.resize(m_clients.size() - deadClients);
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
    U64 ts = sample.timestampUs;
    m_dataframe->SOC_set(ts / 1'000'000);
    m_dataframe->FRACSEC_set(ts % 1'000'000);
    for (USize i = 0; i < CountSignals; ++i) {
        m_station->PHASOR_VALUE_set(estimation.phasors[i], i);
    }
    m_station->FREQ_set(estimation.frequencies[0]);
    m_station->DFREQ_set(estimation.rocofs[0]);
}

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);
    updateSampleReader();
    m_sender = new PhasorSender();
    m_sender->moveToThread(this);
}

void DataProcessor::updateSampleReader()
{
    SamplerSettings newSettings;
    newSettings.load();
    auto newSampler = new SampleReader(newSettings);

    QMutexLocker locker(&m_mutex);
    m_newReader = newSampler;
    /// `moveToThread` must be called from the thread where the object was
    /// created, so we do it here
    m_newReader->moveToThread(this); /// Move the new sampler to `this`, a `QThread` object
}

void DataProcessor::run()
{
    while (true) {
        if (m_newReader) {
            {
                QMutexLocker locker(&m_mutex);
                std::swap(m_reader, m_newReader);
            }
            delete m_newReader;
            m_newReader = nullptr;
        }

        if (m_reader) {
            auto oldState = m_reader->state();
            auto nread = m_reader->attemptRead(m_sampleReadBuffer);
            auto newState = m_reader->state();

            if (newState & SampleReader::DataValid) {
                for (USize i = 0; i < nread; ++i) {
                    const auto &sample = m_sampleReadBuffer[i];
                    for (USize j = 1; j < SampleStoreBufferSize; ++j) {
                        m_sampleStoreBuffer[j - 1] = m_sampleStoreBuffer[j];
                    }
                    m_sampleStoreBuffer[SampleStoreBufferSize - 1] = sample;
                    m_estimator->updateEstimation(sample);
                    m_sender->attemptSend(sample, m_estimator->currentEstimation());
                }
            }
            if (oldState != newState) {
                emit sampleReaderStateChanged(newState);
            }
        }
    }
}
