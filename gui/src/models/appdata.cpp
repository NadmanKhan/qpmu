#include <QtMath>

#include "appdata.h"
#include "signaldatamodel.h"

AppData::AppData(QObject *parent) : QObject(parent)
{
    // Create signal list model
    m_signalDataModel = new SignalDataModel(this);

    // Connect signals from signal model to propagate data updates
    connect(m_signalDataModel, &QAbstractListModel::dataChanged, this, &AppData::dataUpdated);

    m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // Setup simulation timer for testing
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(100); // 10 Hz update rate
    connect(m_simulationTimer, &QTimer::timeout, this, &AppData::updateSimulatedData);
}

qreal AppData::systemFrequency() const
{
    if (m_signalDataModel->rowCount() > 0) {
        QVariant frequencyVariant = m_signalDataModel->data(m_signalDataModel->index(0, 0),
                                                            SignalDataModel::FrequencyRole);
        if (frequencyVariant.canConvert<qreal>()) {
            return frequencyVariant.value<qreal>();
        }
    }
    return 0.0;
}

void AppData::togglePause()
{
    m_isPaused = !m_isPaused;
    emit pauseStateChanged();
}

void AppData::startSimulation()
{
    m_simulationTimer->start();
}

void AppData::updateSimulatedData()
{
    m_simulationTime += 0.1; // Advance time

    // Delegate to signal model (pause state now managed by SignalDataModel)
    m_signalDataModel->updateSimulatedData(m_simulationTime);

    if (!m_signalDataModel->isPaused()) {
        m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}

void AppData::processFrame(const qpmu::Measurement_Frame &frame)
{
    // Delegate to signal model (pause state now managed by SignalDataModel)
    m_signalDataModel->updateFromFrame(frame);

    if (!m_signalDataModel->isPaused()) {
        m_lastSampleTime = QDateTime::fromMSecsSinceEpoch(frame.sample_frame.timestamp
                                                          / (qpmu::Time_Resolution / 1000))
                                   .toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}
