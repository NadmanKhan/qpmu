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

Sampler::Sampler(const SamplerSettings &settings) : m_settings(settings)
{
    if (m_settings.connection == SamplerSettings::Socket) {
        QAbstractSocket *socket;
        if (m_settings.socketConfig.socketType == SamplerSettings::TcpSocket)
            socket = new QTcpSocket(this);
        else
            socket = new QUdpSocket(this);
        m_device = socket;
        socket->connectToHost(m_settings.socketConfig.host, m_settings.socketConfig.port);
        m_waitForConnected = [=] { return socket->waitForConnected(m_connectionWaitTime); };
    } else if (m_settings.connection == SamplerSettings::Process) {
        auto process = new QProcess(this);
        m_device = process;
        process->setProgram(m_settings.processConfig.prog);
        process->setArguments(m_settings.processConfig.args);
        process->start(QProcess::ReadOnly);
        m_waitForConnected = [=] { return process->waitForStarted(m_connectionWaitTime); };
    }

    if (settings.connection != SamplerSettings::None) {
        if (settings.isDataBinary) {
            m_readSample = [this](qpmu::Sample &sample) {
                auto bytesRead = m_device->read((char *)&sample, sizeof(Sample));
                return (bytesRead == sizeof(Sample));
            };
        } else {
            m_readSample = [this](qpmu::Sample &sample) {
                char line[1000];
                bool ok = true;
                if (m_device->readLine(line, sizeof(line)) > 0) {
                    std::string error;
                    sample = parseSample(line, &error);
                    if (!error.empty()) {
                        ok = false;
                        qInfo() << "Error parsing sample: " << error.c_str();
                        qInfo() << "Sample line:   " << line;
                        qInfo() << "Parsed sample: " << toString(sample).c_str();
                    }
                }
                return ok;
            };
        }
    }
}

void Sampler::work(qpmu::Sample &outSample)
{
    int newState = 0;
    /// Check if enabled
    newState |= (Enabled * bool(m_settings.connection != SamplerSettings::None));
    /// Check if connected
    newState |= (Connected * bool((newState & Enabled) && m_waitForConnected()));
    /// Check if reading
    newState |= (DataReading
                 * bool((newState & Connected) && m_device->waitForReadyRead(m_readWaitTime)));
    /// Check if valid
    newState |= (DataValid * bool((newState & DataReading) && m_readSample(outSample)));
    m_state = newState;
}

DataProcessor::DataProcessor() : QThread()
{
    m_estimator = new PhasorEstimator(50, 1200);
    updateSamplerConnection();
}

void DataProcessor::updateSamplerConnection()
{
    SamplerSettings newSettings;
    newSettings.load();
    auto newSampler = new Sampler(newSettings);

    QMutexLocker locker(&m_mutex);
    m_newSampler = newSampler;
    /// `moveToThread` must be called from the thread where the object was
    /// created, so we do it here
    m_newSampler->moveToThread(this); /// Move the new sampler to `this`, a `QThread` object
}

void DataProcessor::run()
{
    Sample sample;

    while (true) {
        if (m_newSampler) {
            {
                QMutexLocker locker(&m_mutex);
                std::swap(m_sampler, m_newSampler);
            }
            delete m_newSampler;
            m_newSampler = nullptr;
        }

        if (m_sampler) {
            m_sampler->work(sample);
            if (m_sampler->state() & Sampler::DataValid) {
                m_estimator->updateEstimation(sample);
            }
        }
    }
}
