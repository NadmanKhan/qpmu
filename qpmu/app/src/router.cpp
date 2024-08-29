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

    USize fs = settings.value("SamplingFrequency", (quint64)conf::SamplingFrequency).toUInt();
    USize f0 = settings.value("NominalFrequency", (quint64)conf::NominalFrequency).toUInt();
    Q_ASSERT(fs % f0 == 0);
    USize windowSize = fs / f0;

    std::array<std::pair<FloatType, FloatType>, CountSignals> calibParams;

    for (USize i = 0; i < CountSignals; ++i) {
        auto factor = settings.value(QString("SignalConfig/%1/CalibFactor")
                                             .arg(conf::SignalConfigs[i].name),
                                     conf::SignalConfigs[i].calib_factor)
                              .toFloat();
        auto offset = settings.value(QString("SignalConfig/%1/CalibOffset")
                                             .arg(conf::SignalConfigs[i].name),
                                     conf::SignalConfigs[i].calib_offset)
                              .toFloat();
        calibParams[i] = std::make_pair(factor, offset);
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
    m_sampleSourceConfig.format_is_binary =
            settings.value("Sampling/FormatIsBinary", conf::SampleSourceConfig.format_is_binary)
                    .toBool();

    char const *const SourceTypeStr =
            conf::SampleSourceConfig.type == SampleSourceConfig::Type::Stdin  ? "stdin"
            : conf::SampleSourceConfig.type == SampleSourceConfig::Type::Tcp  ? "tcp"
            : conf::SampleSourceConfig.type == SampleSourceConfig::Type::Udp  ? "udp"
            : conf::SampleSourceConfig.type == SampleSourceConfig::Type::File ? "file"
                                                                              : "process";

    auto sourceTypeStr = settings.value("Sampling/SourceType", SourceTypeStr).toString().toLower();

    m_sampleSourceConfig.type = (sourceTypeStr == "stdin") ? SampleSourceConfig::Type::Stdin
            : (sourceTypeStr == "tcp")                     ? SampleSourceConfig::Type::Tcp
            : (sourceTypeStr == "udp")                     ? SampleSourceConfig::Type::Udp
            : (sourceTypeStr == "file")                    ? SampleSourceConfig::Type::File
                                                           : SampleSourceConfig::Type::Process;

    QMutexLocker locker(&m_mutex);

    switch (m_sampleSourceConfig.type) {

    case SampleSourceConfig::Type::Stdin: {
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
    } break;

    case SampleSourceConfig::Type::Tcp:
    case SampleSourceConfig::Type::Udp: {
        m_sampleSourceConfig.handle.socket.host =
                settings.value("Sampling/Host", conf::SampleSourceConfig.handle.socket.host)
                        .toString()
                        .toUtf8()
                        .data();
        m_sampleSourceConfig.handle.socket.port =
                settings.value("Sampling/Port", conf::SampleSourceConfig.handle.socket.port)
                        .toUInt();

        auto host = m_sampleSourceConfig.handle.socket.host;
        auto port = m_sampleSourceConfig.handle.socket.port;

        QAbstractSocket *socket;
        if (m_sampleSourceConfig.type == SampleSourceConfig::Type::Tcp) {
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

    } break;

    case qpmu::SampleSourceConfig::Type::File:
    case qpmu::SampleSourceConfig::Type::Process: {
        auto path = settings.value("Sampling/Path").toString();
        auto args = settings.value("Sampling/Args").toString();

        /// If the file does not exist, the program will issue a warning and continue
        if (!QFile::exists(path)) {
            qCritical() << "The file" << path << "does not exist";
        }
        break;

        /// Split the arguments into a list
        QStringList argList;
        {
            QString arg;
            bool inQuote = false;
            for (auto c : args) {
                if (c == ' ' && !inQuote) {
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

        m_sampleSourceConfig.handle.file.path = path.toUtf8().data();
        m_sampleSourceConfig.handle.file.args = args.toUtf8().data();

        if (m_sampleSourceConfig.type == SampleSourceConfig::Type::Process) {
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

        } else {
            auto file = new QFile(path);
            newDevice = file;

            newDeviceReady = [file]() {
                if (!file->open(QIODevice::ReadOnly)) {
                    qCritical() << "Failed to open the file";
                    return false;
                }
                return true;
            };
        }
    } break;
    default:
        qFatal("Invalid sample source type");
    }

    if (newDevice) {
        std::swap(m_sampleSourceDevice, newDevice);
        std::swap(m_sampleSourceDeviceReady, newDeviceReady);
    } else {
        qCritical("Failed to create a new sample source device");
    }
}

qpmu::Synchrophasor Router::currentSynchrophasor()
{
    qpmu::Synchrophasor synchrophasor;
    {
        QMutexLocker locker(&m_mutex);
        synchrophasor = m_synchrophasor;
    }
    return synchrophasor;
}

void Router::run()
{
    using namespace qpmu;
    Sample sample;
    char *line = nullptr;
    qint64 line_length;
    bool sample_is_obtained = false;

    while (true) {
        if (m_sampleSourceDeviceReady && m_sampleSourceDeviceReady()) {
            sample_is_obtained = false;

            if (m_sampleSourceConfig.format_is_binary) {
                if (m_sampleSourceDevice->read((char *)&sample, sizeof(Sample)) == sizeof(Sample)) {
                    m_estimator->update_estimation(sample);
                    sample_is_obtained = true;
                }
            } else {
                if ((line_length = m_sampleSourceDevice->readLine(line, 1000)) > 0) {
                    sample_is_obtained = util::parse_as_sample(sample, line);
                }
            }

            if (sample_is_obtained) {
                QMutexLocker locker(&m_mutex);
                m_synchrophasor = m_estimator->synchrophasor();
            } else {
                sample_is_obtained = false;
            }
        } else {
            /// If the sample source is not ready, sleep for a while
            msleep(100);
        }
    }
}
