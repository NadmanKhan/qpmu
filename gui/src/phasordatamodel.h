#ifndef PHASOR_DATA_MODEL_H
#define PHASOR_DATA_MODEL_H

#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <QDateTime>

namespace qpmu {
    struct Measurement_Frame;
}

class PhasorDataModel : public QObject
{
    Q_OBJECT

    // Phasor data properties
    Q_PROPERTY(QVariantList phasorMagnitudes READ phasorMagnitudes NOTIFY dataUpdated)
    Q_PROPERTY(QVariantList phasorPhases READ phasorPhases NOTIFY dataUpdated)
    Q_PROPERTY(QVariantList frequencies READ frequencies NOTIFY dataUpdated)
    Q_PROPERTY(qreal samplingRate READ samplingRate NOTIFY dataUpdated)
    Q_PROPERTY(QString lastSampleTime READ lastSampleTime NOTIFY dataUpdated)

    // UI state properties
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY pauseStateChanged)

    // Signal colors
    Q_PROPERTY(QVariantList signalColors READ signalColors CONSTANT)
    Q_PROPERTY(QVariantList signalNames READ signalNames CONSTANT)

public:
    explicit PhasorDataModel(QObject *parent = nullptr);

    // Property getters
    QVariantList phasorMagnitudes() const { return m_phasorMagnitudes; }
    QVariantList phasorPhases() const { return m_phasorPhases; }
    QVariantList frequencies() const { return m_frequencies; }
    qreal samplingRate() const { return m_samplingRate; }
    QString lastSampleTime() const { return m_lastSampleTime; }
    bool isPaused() const { return m_isPaused; }
    QVariantList signalColors() const;
    QVariantList signalNames() const;

    // Invokable methods
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void startSimulation(); // For testing without real data source

signals:
    void dataUpdated();
    void pauseStateChanged();

private slots:
    void updateSimulatedData(); // For testing

private:
    void processFrame(const qpmu::Measurement_Frame &frame);
    void updateProperties(const QVariantList &magnitudes, const QVariantList &phases,
                         const QVariantList &freqs, qreal samplingRate, qint64 timestamp);

    // Data storage
    QVariantList m_phasorMagnitudes;
    QVariantList m_phasorPhases;
    QVariantList m_frequencies;
    qreal m_samplingRate = 0.0;
    QString m_lastSampleTime;
    bool m_isPaused = false;

    // Simulation timer (for testing)
    QTimer *m_simulationTimer = nullptr;
    qreal m_simulationTime = 0.0;
};

#endif // PHASOR_DATA_MODEL_H
