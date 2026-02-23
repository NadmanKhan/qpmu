#include "applicationdatamodel.h"
#include <QtMath>

ApplicationDataModel::ApplicationDataModel(QObject *parent)
    : QObject(parent)
{
    // Create signal list model
    m_signalDataModel = new SignalDataModel(this);

    // Connect signals from signal model to propagate data updates
    connect(m_signalDataModel, &QAbstractListModel::dataChanged,
            this, &ApplicationDataModel::dataUpdated);

    m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // Setup simulation timer for testing
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(100); // 10 Hz update rate
    connect(m_simulationTimer, &QTimer::timeout, this, &ApplicationDataModel::updateSimulatedData);
}

qreal ApplicationDataModel::systemFrequency() const
{
    if (m_signalDataModel->rowCount() > 0) {
        // Get the first SignalData object and access its frequency property
        QVariant signalVariant = m_signalDataModel->data(m_signalDataModel->index(0, 0),
                                                     SignalDataModel::SignalDataRole);
        SignalData *signal = signalVariant.value<SignalData*>();
        if (signal) {
            return signal->frequency();
        }
    }
    return 60.0;
}

void ApplicationDataModel::togglePause()
{
    m_isPaused = !m_isPaused;
    emit pauseStateChanged();
}

void ApplicationDataModel::startSimulation()
{
    m_simulationTimer->start();
}

void ApplicationDataModel::updateSimulatedData()
{
    m_simulationTime += 0.1; // Advance time

    // Delegate to signal model
    m_signalDataModel->updateSimulatedData(m_simulationTime, m_isPaused);

    if (!m_isPaused) {
        m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}

void ApplicationDataModel::processFrame(const qpmu::Measurement_Frame &frame)
{
    // Delegate to signal model
    m_signalDataModel->updateFromFrame(frame, m_isPaused);

    if (!m_isPaused) {
        m_lastSampleTime = QDateTime::fromMSecsSinceEpoch(frame.timestamp / 1000000)
                              .toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}
