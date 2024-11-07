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

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);

    if (APP->arguments().contains("--binary")) {
        qDebug() << "Reading data as binary";
        if (APP->arguments().contains("--rpmsg")) {
            auto adc_stream = qgetenv("ADC_STREAM");
            qDebug() << "Reading raw data buffer (in binary) from RPMsg device: " << adc_stream;
            auto fd = open(adc_stream, O_RDWR);
            if (fd < 0) {
                perror("Failed to open RPMsg device");
                qFatal("Failed to open RPMsg device\n");
            }

            constexpr uint64_t TimeQueryPoint = 1024;
            m_readSamples = [=](QString &error) -> uint64_t {
                if (write(fd, 0, 0) < 0) {
                    error = QStringLiteral("Failed to write to RPMsg device: %1")
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
                    for (uint64_t i = 0; i < CountSignals; ++i) {
                        s.channels[i] = m_rawReadState.streamBuf.data[i];
                    }
                    qDebug() << "Sample: " << toString(s).c_str();
                    m_rawReadState.counter += 1;
                    m_rawReadState.lastTimeUsec = s.timestampUsec;
                    m_rawReadState.lastBufTimeNsec = m_rawReadState.streamBuf.timestampNsec;
                    return 1;
                } else {
                    error = QStringLiteral("Expected %1 bytes, got %2 bytes")
                                    .arg(sizeof(ADCStreamBuffer))
                                    .arg(nread);
                    return 0;
                }
            };
        } else {
            qDebug() << "Reading processed samples (in binary) from stdin";
            m_readSamples = [=](QString &) -> uint64_t {
                auto nread = fread(m_sampleReadBuffer.data(), sizeof(Sample),
                                   std::tuple_size<SampleReadBuffer>(), stdin);
                return nread;
            };
        }
    } else {
        qFatal("Not implemented\n");
    }

    m_sender = new PhasorSender();
    m_sender->start();
}

void DataProcessor::replacePhasorSender()
{
    QMutexLocker locker(&m_mutex);
    if (m_sender) {
        m_sender->stopRunning();
        connect(m_sender, &QThread::finished, m_sender, &QObject::deleteLater);
    }
    m_sender = new PhasorSender();
    m_sender->start();
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

        for (size_t i = 0; i < nread; ++i) {
            const auto &sample = m_sampleReadBuffer[i];

            QMutexLocker locker(&m_mutex);

            for (size_t j = 1; j < m_sampleStore.size(); ++j) {
                m_sampleStore[j - 1] = m_sampleStore[j];
            }
            m_sampleStore.back() = sample;
            m_estimator->updateEstimation(sample);
        }

        m_sender->updateData(m_estimator->currentSample(), m_estimator->currentEstimation());
    }
}
