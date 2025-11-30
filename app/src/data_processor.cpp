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
#include <c37118command.h>
#include <fcntl.h>
#include <qdir.h>
#include <qstringliteral.h>
#include <unistd.h>

using namespace qpmu;

template <class T>
T median(std::vector<T> vec)
{
    assert(!vec.empty());
    std::sort(vec.begin(), vec.end());
    size_t n = vec.size();
    if (n % 2 == 1) {
        return vec[n / 2];
    } else {
        return (vec[n / 2 - 1] + vec[n / 2]) / 2;
    }
}

const Estimation &DataProcessor::lastEstimation()
{
    QMutexLocker locker(&m_mutex);
    return m_estimations.back();
}

/// Get estimation after running median filters
const Estimation DataProcessor::lastEstimationFiltered()
{
    std::vector<Float> filterableMagnitudes[CountSignals];
    Estimation result;

    {
        QMutexLocker locker(&m_mutex);
        result = m_estimations.back();
        for (size_t i = 0; i < CountSignals; ++i) {
            size_t medianWindowSize =
                    std::min(TypeOfSignal[i] == VoltageSignal ? 100ul : 32ul, m_estimations.size());
            filterableMagnitudes[i].resize(medianWindowSize);
            size_t offset = m_estimations.size() - medianWindowSize;
            for (size_t j = 0; j < medianWindowSize; ++j) {
                filterableMagnitudes[i][j] = std::abs(m_estimations[j + offset].phasors[i]);
            }
        }
    }

    for (size_t i = 0; i < CountSignals; ++i) {
        auto medianMagnitude = median(filterableMagnitudes[i]);
        result.phasors[i] = std::polar(medianMagnitude, std::arg(result.phasors[i]));
    }
    return result;
}

const qpmu::Sample &DataProcessor::lastSample()
{
    QMutexLocker locker(&m_mutex);
    return m_samples.back();
}

const SampleWindow &DataProcessor::sampleWindow()
{
    QMutexLocker locker(&m_mutex);
    return m_samples;
}

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);

    if (APP->arguments().contains("--binary") || APP->arguments().contains("-b")) {
        m_readBinary = true;
        qDebug() << "Reading processed samples (in binary) from stdin";
    } else {
        qFatal("Not implemented\n");
    }

    auto adcStreamPath = qgetenv("ADC_STREAM");
    if (!adcStreamPath.isEmpty()) {
        qDebug() << "Reading from the adc stream device: " << adcStreamPath;
        inputFile = fopen(adcStreamPath.constData(), "rb");
        if (!inputFile) {
            qFatal("Failed to open ADC stream device");
        }
    }

    m_server = new PhasorServer();
    m_serverThread = new QThread();
    m_server->moveToThread(m_serverThread);
    m_serverThread->start();
}

void DataProcessor::replacePhasorServer()
{
    m_server->deleteLater();
    m_server = new PhasorServer();
    m_server->moveToThread(m_serverThread);
}

void DataProcessor::run()
{
    while (true) {

        QString error;
        auto nread = readSamples(error);
        if (!error.isEmpty()) {
            qWarning() << error;
            continue;
        }

        for (size_t i = 0; i < nread; ++i) {
            const auto &sample = m_sampleReadBuffer[i];

            QMutexLocker locker(&m_mutex);

            /// shift buffer and add new sample
            for (size_t j = 1; j < m_samples.size(); ++j) {
                m_samples[j - 1] = m_samples[j];
            }
            m_samples.back() = sample;
            qDebug() << QString::fromStdString(toString(sample));

            /// shift buffer and add new estimation
            for (size_t j = 1; j < m_estimations.size(); ++j) {
                m_estimations[j - 1] = m_estimations[j];
            }
            m_estimator->updateEstimation(sample);
            m_estimations.back() = m_estimator->currentEstimation();
        }
    }
}

uint64_t DataProcessor::readSamples(QString &error) const
{
    uint64_t nread = 0;
    if (m_readBinary) {
        nread = fread((void *)m_sampleReadBuffer.data(), sizeof(Sample),
                      std::tuple_size<SampleReadBuffer>(), inputFile);
        if (nread < std::tuple_size<SampleReadBuffer>()) {
            if (feof(inputFile)) {
                error = "End of input stream reached";
            } else if (ferror(inputFile)) {
                error = "Error reading from input stream";
            }
        }
    }
    return nread;
}
