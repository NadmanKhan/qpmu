#include "application.h"
#include "ipc/guiclient.h"
#include <QDateTime>
#include <QDebug>

Application::Application(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
    // Create data model
    m_signalDataModel = new SignalDataModel(this);
    // Note: dataUpdated is emitted explicitly in processFrame/updateSimulatedData
    // after updating m_lastSampleTime — no auto-forward from dataChanged needed.
    connect(m_signalDataModel, &SignalDataModel::pauseStateChanged,
            this, &Application::pauseStateChanged);

    // Create IPC client
    m_ipcClient = new GuiIpcClient(this);
    connect(m_ipcClient, &GuiIpcClient::frameReceived,
            this, &Application::onIPCFrameReceived);
    connect(m_ipcClient, &GuiIpcClient::connected,
            this, &Application::onIPCConnected);
    connect(m_ipcClient, &GuiIpcClient::disconnected,
            this, &Application::onIPCDisconnected);

    // Setup simulation timer
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(500); // 2 Hz
    connect(m_simulationTimer, &QTimer::timeout,
            this, &Application::updateSimulatedData);

    // Throttle IPC frames to 2 Hz — stores latest frame, processes on timer tick
    m_ipcThrottleTimer = new QTimer(this);
    m_ipcThrottleTimer->setInterval(500); // 2 Hz
    connect(m_ipcThrottleTimer, &QTimer::timeout, this, [this]() {
        if (m_hasPendingFrame) {
            processFrame(m_pendingFrame);
            m_hasPendingFrame = false;
        }
    });

    // Auto-connect after event loop starts
    QTimer::singleShot(100, this, [this]() {
        m_ipcClient->connectToService();
    });

    m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}

bool Application::isConnected() const {
    return m_ipcClient && m_ipcClient->isConnected();
}

qreal Application::systemFrequency() const {
    if (m_signalDataModel->rowCount() > 0) {
        QVariant freq = m_signalDataModel->data(
            m_signalDataModel->index(0, 0),
            SignalDataModel::FrequencyRole);
        if (freq.canConvert<qreal>()) {
            return freq.value<qreal>();
        }
    }
    return 0.0;
}

bool Application::isPaused() const {
    return m_signalDataModel->isPaused();
}

void Application::togglePause() {
    m_signalDataModel->setIsPaused(!m_signalDataModel->isPaused());
}

void Application::startSimulation() {
    m_simulationTimer->start();
}

void Application::processFrame(const qpmu::Measurement_Frame& frame) {
    m_signalDataModel->updateFromFrame(frame);

    if (!m_signalDataModel->isPaused()) {
        m_lastSampleTime = QDateTime::fromMSecsSinceEpoch(
            frame.timestamp / (qpmu::Time_Resolution / 1000))
            .toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}

void Application::onIPCFrameReceived(const qpmu::Measurement_Frame& frame) {
    if (m_simulationTimer->isActive()) {
        m_simulationTimer->stop();
    }
    m_liveMode = true;

    // Buffer the latest frame; the throttle timer processes it at 2 Hz
    m_pendingFrame = frame;
    m_hasPendingFrame = true;
    if (!m_ipcThrottleTimer->isActive()) {
        // Process the first frame immediately, then throttle subsequent ones
        processFrame(m_pendingFrame);
        m_hasPendingFrame = false;
        m_ipcThrottleTimer->start();
    }
}

void Application::onIPCConnected() {
    qDebug() << "Application: Connected to QPMU service";
    emit connectionStateChanged();
}

void Application::onIPCDisconnected() {
    qDebug() << "Application: Disconnected from QPMU service - falling back to simulation";
    m_liveMode = false;
    m_ipcThrottleTimer->stop();
    m_hasPendingFrame = false;

    if (!m_simulationTimer->isActive()) {
        startSimulation();
    }

    emit connectionStateChanged();
}

void Application::updateSimulatedData() {
    m_simulationTime += 0.5;
    m_signalDataModel->updateSimulatedData(m_simulationTime);

    if (!m_signalDataModel->isPaused()) {
        m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit dataUpdated();
    }
}
