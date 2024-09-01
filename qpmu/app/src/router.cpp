#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QMutexLocker>

#include <array>
#include <cctype>
#include <iostream>

#include "qpmu/common.h"
#include "app.h"
#include "router.h"

using namespace qpmu;

Router::Router() : QThread()
{
    using namespace qpmu;
    const auto &settings = *APP->settings();

    /// Initialize the Estimator object

    USize fs = settings.get(Settings::list.fundamentals.adc.samplingFrequency).toUInt();
    USize f0 = settings.get(Settings::list.fundamentals.powerSource.nominalFrequency).toUInt();
    Q_ASSERT(fs % f0 == 0);
    USize windowSize = fs / f0;

    std::array<std::pair<Float, Float>, CountSignals> calibParams;

    for (USize i = 0; i < CountSignals; ++i) {
        auto gain = settings.get(Settings::list.sampling.calibration[i * 2 + 0]).toFloat();
        auto bias = settings.get(Settings::list.sampling.calibration[i * 2 + 1]).toFloat();
        calibParams[i] = std::make_pair(gain, bias);
    }

    m_estimator = new Estimator(windowSize, calibParams);

    updateSampleSource();
}

void Router::updateSampleSource()
{
    using namespace qpmu;

    if (m_sampleSourceDevice) {
        m_sampleSourceDevice->close();
    }
    QIODevice *newDevice = nullptr;
    std::function<bool()> newDeviceReady = nullptr;

    const auto &settings = *APP->settings();

    auto sourceType = settings.get(Settings::list.sampling.inputSource.type).toString().toLower();

    if (sourceType == "stdin") {
        auto file = new QFile();
        newDevice = file;
        file->open(stdin, QIODevice::ReadOnly);

        newDeviceReady = [file]() {
            if (!file->isOpen()) {
                qCritical() << "Failed to open stdin";
                return false;
            }
            return true;
        };

    } else if (sourceType == "tcp socket" || sourceType == "udp socket") {
        const bool isTcp = sourceType == "tcp socket";

        auto host = settings.get(Settings::list.sampling.inputSource.host).toString();
        auto port = settings.get(Settings::list.sampling.inputSource.port).toUInt();

        QAbstractSocket *socket;
        if (isTcp) {
            socket = new QTcpSocket();
        } else {
            socket = new QUdpSocket();
        }
        newDevice = socket;
        socket->connectToHost(host, port);

        newDeviceReady = [socket]() {
            if (!socket->waitForConnected()) {
                qCritical() << "Failed to connect to the TCP server";
                return false;
            }
            if (!socket->waitForReadyRead()) {
                qCritical() << "Failed to receive data from the TCP server";
                return false;
            }
            return true;
        };
    } else if (sourceType == "file" || sourceType == "process") {
        const bool isFile = sourceType == "file";
        auto path = settings.get(Settings::list.sampling.inputSource.path).toString();
        auto args = settings.get(Settings::list.sampling.inputSource.args).toString();

        /// If the file does not exist, the program will issue a warning and continue
        if (!QFile::exists(path)) {
            qCritical() << "The file" << path << "does not exist";
        }

        /// Split the arguments into a list
        QStringList argList;
        {
            QString arg;
            bool inQuote = false;
            for (auto c : args) {
                if (c == ' ' && !inQuote && !arg.isEmpty()) {
                    argList.append(arg);
                    arg.clear();
                } else if (c == '"') {
                    inQuote = !inQuote;
                } else {
                    arg.append(c);
                }
            }
            if (!arg.isEmpty()) {
                argList.append(arg);
            }
        }

        if (isFile) {
            auto file = new QFile(path);
            newDevice = file;
            newDeviceReady = [file]() {
                if (!file->open(QIODevice::ReadOnly)) {
                    qCritical() << "Failed to open the file";
                    return false;
                }
                return true;
            };
        } else {
            auto process = new QProcess();
            newDevice = process;
            process->setProgram(path);
            process->setArguments(argList);
            newDeviceReady = [process]() {
                if (!process->waitForStarted()) {
                    qCritical() << "Failed to start the process";
                    return false;
                }
                if (!process->waitForReadyRead()) {
                    qCritical() << "Failed to receive data from the process";
                    return false;
                }
                return true;
            };
        }
    } else {
        qFatal("Invalid sample source type");
    }

    if (newDevice) {
        QMutexLocker locker(&m_mutex);
        std::swap(m_sampleSourceDevice, newDevice);
        std::swap(m_sampleSourceDeviceReady, newDeviceReady);
        m_sampleSourceIsBinary =
                settings.get(Settings::list.sampling.inputSource.isBinary).toBool();
    } else {
        qCritical("Failed to create a new sample source device");
    }
}

qpmu::Synchrophasor Router::lastSynchrophasor()
{
    qpmu::Synchrophasor synchrophasor;
    {
        QMutexLocker locker(&m_mutex);
        synchrophasor = m_estimator->synchrophasor();
    }
    return synchrophasor;
}

void Router::run()
{
    using namespace qpmu;
    char *line = nullptr;
    qint64 line_length;
    bool sample_is_obtained = false;

    while (true) {
        if (m_sampleSourceDeviceReady && m_sampleSourceDeviceReady()) {
            sample_is_obtained = false;

            if (m_sampleSourceIsBinary) {
                if (m_sampleSourceDevice->read((char *)&m_sample, sizeof(Sample))
                    == sizeof(Sample)) {
                    sample_is_obtained = true;
                }
            } else {
                if ((line_length = m_sampleSourceDevice->readLine(line, 1000)) > 0) {
                    sample_is_obtained = util::parse_as_sample(m_sample, line);
                }
            }

            if (sample_is_obtained) {
                emit newSampleObtained(m_sample);

                Synchrophasor syncph;
                {
                    QMutexLocker locker(&m_mutex);
                    m_estimator->update_estimation(m_sample);
                    syncph = m_estimator->synchrophasor();
                }
                emit newSynchrophasorObtained(syncph);

            } else {
                qCritical() << "Failed to obtain a sample";
            }
        } else {
            /// If the sample source is not ready, sleep for a while
            msleep(100);
        }
    }
}
