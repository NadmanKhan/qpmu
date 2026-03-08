#ifndef APP_DATA_H
#define APP_DATA_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QItemSelectionModel>
#include <QtQmlIntegration>

#include "qpmu/core.h"
#include "signaldatamodel.h"

// Application-level model managing application metadata
// Contains a SignalDataModel for signal data
class AppData : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(SignalDataModel *signalDataModel READ signalDataModel CONSTANT)
    Q_PROPERTY(qreal samplingRate READ samplingRate NOTIFY dataUpdated)
    Q_PROPERTY(QString lastSampleTime READ lastSampleTime NOTIFY dataUpdated)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pauseStateChanged)
    Q_PROPERTY(qreal systemFrequency READ systemFrequency NOTIFY dataUpdated)

public:
    explicit AppData(QObject *parent = nullptr);

    // Property getters
    inline SignalDataModel *signalDataModel() const { return m_signalDataModel; }
    inline qreal samplingRate() const { return m_samplingRate; }
    inline QString lastSampleTime() const { return m_lastSampleTime; }
    inline bool isPaused() const { return m_isPaused; }
    qreal systemFrequency() const;

    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void startSimulation(); // For testing

public slots:
    void processFrame(const qpmu::Measurement_Frame &frame);

signals:
    void dataUpdated();
    void pauseStateChanged();

private slots:
    void updateSimulatedData();

private:
    // Signal list model
    SignalDataModel *m_signalDataModel;

    // Global metadata
    qreal m_samplingRate = 1000.0;
    QString m_lastSampleTime;
    bool m_isPaused = false;

    // Simulation timer (for testing)
    QTimer *m_simulationTimer = nullptr;
    qreal m_simulationTime = 0.0;
};

#endif // APP_DATA_H
