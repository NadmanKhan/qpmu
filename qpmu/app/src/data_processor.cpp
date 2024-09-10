#include "qpmu/defs.h"
#include "qpmu/util.h"
#include "app.h"
#include "data_processor.h"
#include "main_window.h"
#include "src/settings_models.h"

#include <QDebug>
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QMutexLocker>
#include <QLabel>

#include <array>
#include <cctype>
#include <iostream>

using namespace qpmu;

DataProcessor::DataProcessor() : QThread()
{
    /// Initialize the Estimator object

    m_estimator = new PhasorEstimator(50, 1200);

    m_sampleSourcesettings.load();

    updateSampleSource();
}

const Estimation &DataProcessor::lastEstimation()
{
    QMutexLocker locker(&m_mutex);
    return m_estimator->lastEstimation();
}

const Sample &DataProcessor::lastSample()
{
    QMutexLocker locker(&m_mutex);
    return m_estimator->lastSample();
}

void DataProcessor::updateSampleSource()
{
    if (!m_sampleSourcesettings.isValid()) {
        // qCritical() << "Invalid sample source settings";
        // return;
    }

    using namespace qpmu;

    QIODevice *newDevice = nullptr;
    std::function<void()> startNewDevice = nullptr;
    std::function<bool()> isNewDeviceReadyToRead = nullptr;

    if (m_sampleSourcesettings.connection == SampleSourceSettings::SocketConnection) {

        QAbstractSocket *sock;
        if (m_sampleSourcesettings.socketConfig.socketType == SampleSourceSettings::TcpSocket) {
            sock = new QTcpSocket();
        } else if (m_sampleSourcesettings.socketConfig.socketType
                   == SampleSourceSettings::UdpSocket) {
            sock = new QUdpSocket();
        } else {
            qFatal("Invalid socket type");
        }

        /// Set new device
        newDevice = sock;

        /// Set the startNewDevice function
        startNewDevice = [=] {
            qDebug() << "Connecting to" << m_sampleSourcesettings.socketConfig.host
                     << m_sampleSourcesettings.socketConfig.port << "using" << sock->socketType();

            sock->connectToHost(m_sampleSourcesettings.socketConfig.host,
                                m_sampleSourcesettings.socketConfig.port);

            qDebug() << "Connected to" << sock->peerName() << "on port" << sock->peerPort()
                     << "using" << sock->socketType();
        };

        /// Set the newDeviceReadyToRead function
        isNewDeviceReadyToRead = [sock]() -> bool {
            if (!sock) {
                return false;
            }

            /// Connect to the host if not already connected
            qDebug() << "... waiting for" << sock->socketType() << "socket to be connected ...";
            if (!sock->waitForConnected(3000)) {
                // qCritical() << "Failed to connect to the" << sock->socketType() << "socket";
                // qCritical() << "🔴 Socket error:" << sock->errorString();
                // qCritical() << "❔ Socket state:" << sock->state();
                return false;
            }
            // qDebug() << "🟢";
            // qDebug() << "❔ Socket state:" << sock->state();

            /// Read the data from the socket when ready
            // qDebug() << "... waiting for data from the" << sock->socketType() << "socket ...";
            if (!sock->waitForReadyRead(3000)) {
                // qCritical() << "Failed to receive data from the" << sock->socketType() <<
                // "socket"; qCritical() << "🔴 Socket error:" << sock->errorString(); qCritical() <<
                // "❔ Socket state:" << sock->state();
                return false;
            }
            // qDebug() << "🟢";
            // qDebug() << "❔ Socket state:" << sock->state();

            return true;
        };

    } else if (m_sampleSourcesettings.connection == SampleSourceSettings::ProcessConnection) {

        auto process = new QProcess();

        /// Set new device
        newDevice = process;

        /// Set the startNewDevice function
        startNewDevice = [=] {
            process->setProgram(m_sampleSourcesettings.processConfig.prog);
            process->setArguments(m_sampleSourcesettings.processConfig.args);
            qDebug() << "Starting new device: Starting the process"
                     << m_sampleSourcesettings.processConfig.prog
                     << m_sampleSourcesettings.processConfig.args;
            process->start(QProcess::ReadOnly);
        };

        /// Set the newDeviceReadyToRead function
        isNewDeviceReadyToRead = [process]() -> bool {
            if (!process) {
                return false;
            }

            /// Start the process if not already started
            // qDebug() << "... waiting for process to start ...";
            if (!process->waitForStarted(3000)) {
                // qCritical() << "Failed to start the process";
                // qCritical() << "🔴 Process error:" << process->errorString();
                // qCritical() << "❔ Process state:" << process->state();
                return false;
            }
            // qDebug() << "🟢";
            // qDebug() << "❔ Process state:" << process->state();

            /// Read the data from the process when ready
            // qDebug() << "... waiting for data from the process ...";
            if (!process->waitForReadyRead(3000)) {
                // qCritical() << "Failed to receive data from the process";
                // qCritical() << "🔴 Process error:" << process->errorString();
                // qCritical() << "❔ Process state:" << process->state();
                return false;
            }
            // qDebug() << "🟢";
            // qDebug() << "❔ Process state:" << process->state();

            return true;
        };
    }

    if (newDevice) {
        QMutexLocker locker(&m_mutex);
        std::swap(sampler.newDevice, newDevice);
        std::swap(sampler.startNewDevice, startNewDevice);
        std::swap(sampler.isDeviceReadyToRead, isNewDeviceReadyToRead);
        sampler.isDataBinary = m_sampleSourcesettings.isDataBinary;
        sampler.newDevice->moveToThread(this);
        qDebug() << "Sample source updated";

    } else {
        qCritical() << "Failed to create a new sample source device";
    }
}

void DataProcessor::run()
{
    char *line = nullptr;
    qint64 lineLength;
    bool isSampleObtained = false;

    while (true) {
        if (m_sampleSourcesettings.connection == SampleSourceSettings::NoConnectioin) {
            if (sampler.isConnected) {
                sampler.isConnected = false;
                emit sampleSourceConnectionChanged(false);
            }
            qWarning() << "Connection is turned off, sleeping for a while";
            msleep(100);
            continue;
        }

        if (sampler.startNewDevice) { /// Start the device if it is not already started
            qDebug() << "Starting the device";
            if (sampler.device && sampler.device->isOpen()) {
                /// Close the old device first
                qDebug() << "But first, closing the old device";
                sampler.device->deleteLater();
                sampler.device->close();
            }

            /// Start the new device and unset the startDevice function
            sampler.device = sampler.newDevice;
            sampler.startNewDevice();
            sampler.startNewDevice = nullptr;
        }

        if (sampler.isDeviceReadyToRead && sampler.isDeviceReadyToRead()) {
            if (!sampler.isConnected) {
                sampler.isConnected = true;
                emit sampleSourceConnectionChanged(true);
            }

            // qDebug() << "Sample source is ready for read";

            /// If he sample source is ready with for read, then read the sample
            isSampleObtained = false;
            Sample sample;

            if (sampler.isDataBinary) {
                /// If the sample source is binary, read the sample directly
                if (sampler.device->read((char *)&sample, sizeof(Sample)) == sizeof(Sample)) {
                    isSampleObtained = true;
                }
            } else {
                /// Otherwise, read the sample as a string and parse it
                if ((lineLength = sampler.device->readLine(line, 1000)) > 0) {
                    isSampleObtained = parseSample(sample, line);
                }
            }

            if (isSampleObtained) {
                /// If the sample is obtained, update the estimation and emit the signals
                Estimation estimation;
                {
                    QMutexLocker locker(&m_mutex);
                    m_estimator->updateEstimation(sample);
                    estimation = m_estimator->lastEstimation();
                }

                // qDebug() << "Sample obtained";
                // qDebug() << QString::fromStdString(toString(sample));

            } else {
                qCritical() << "Failed to obtain a sample";
            }

        } else {
            if (sampler.isConnected) {
                sampler.isConnected = false;
                emit sampleSourceConnectionChanged(false);
            }

            qWarning() << "Sample source is not ready for read, sleeping for a while";

            msleep(100);
        }
    }
}
