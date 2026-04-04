#ifndef APPLICATION_H
#define APPLICATION_H

#include <QGuiApplication>
#include <QTimer>
#include <QString>

#include "models/signaldatamodel.h"

// Forward declaration to avoid circular dependency
class GuiIpcClient;

class Application : public QGuiApplication {
    Q_OBJECT
    Q_PROPERTY(bool isLiveMode READ isLiveMode NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(QString lastSampleTime READ lastSampleTime NOTIFY dataUpdated)
    Q_PROPERTY(qreal samplingRate READ samplingRate CONSTANT)
    Q_PROPERTY(qreal systemFrequency READ systemFrequency NOTIFY dataUpdated)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pauseStateChanged)

public:
    Application(int &argc, char **argv);
    ~Application() override = default;

    // Global accessor (similar to qApp)
    static Application* instance() {
        return qobject_cast<Application*>(QCoreApplication::instance());
    }

    // Model accessors
    SignalDataModel* signalDataModel() const { return m_signalDataModel; }
    GuiIpcClient* ipcClient() const { return m_ipcClient; }

    // Properties
    bool isLiveMode() const { return m_liveMode; }
    bool isConnected() const;
    QString lastSampleTime() const { return m_lastSampleTime; }
    qreal samplingRate() const { return m_samplingRate; }
    qreal systemFrequency() const;
    bool isPaused() const;

signals:
    void dataUpdated();
    void connectionStateChanged();
    void pauseStateChanged();

public slots:
    void processFrame(const qpmu::Measurement_Frame& frame);
    void togglePause();
    void startSimulation();

private slots:
    void onIPCFrameReceived(const qpmu::Measurement_Frame& frame);
    void onIPCConnected();
    void onIPCDisconnected();
    void updateSimulatedData();

private:
    SignalDataModel* m_signalDataModel;
    GuiIpcClient* m_ipcClient;
    QTimer* m_simulationTimer;

    bool m_liveMode = false;
    QString m_lastSampleTime;
    qreal m_samplingRate = 1000.0;
    qreal m_simulationTime = 0.0;
};

// Global accessor macro (similar to qApp)
#define qpmuApp (Application::instance())

#endif // APPLICATION_H
