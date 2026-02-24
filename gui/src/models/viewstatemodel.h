#ifndef VIEW_STATE_MODEL_H
#define VIEW_STATE_MODEL_H

#include <QObject>
#include <QList>
#include <QtQmlIntegration>

#include "signaldatamodel.h"

// ViewStateModel - Manages UI/view-specific state for LiveMonitor
// Handles magnitude display mode, phase reference, signal visibility, and plot scaling
class ViewStateModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    // Magnitude display modes
    enum MagnitudeMode {
        RMS = 0, // Display RMS magnitude (as-is from estimator)
        Peak = 1 // Display peak magnitude (RMS × √2)
    };
    Q_ENUM(MagnitudeMode)

    // Plot scaling modes
    enum ScalingMode {
        Dynamic = 0, // Auto-scale based on signal magnitudes
        Manual = 1 // User-defined fixed scale
    };
    Q_ENUM(ScalingMode)

    // Data-related properties (affect both table and graphs)
    Q_PROPERTY(MagnitudeMode magnitudeMode READ magnitudeMode WRITE setMagnitudeMode NOTIFY
                       magnitudeModeChanged)
    Q_PROPERTY(int phaseReferenceSignalIndex READ phaseReferenceSignalIndex WRITE
                       setPhaseReferenceSignalIndex NOTIFY phaseReferenceChanged)
    Q_PROPERTY(
            bool isPausedLocal READ isPausedLocal WRITE setIsPausedLocal NOTIFY pauseStateChanged)

    // Plot-related properties (affect only graphs)
    Q_PROPERTY(QList<bool> signalVisibility READ signalVisibility NOTIFY signalVisibilityChanged)
    Q_PROPERTY(ScalingMode voltageScalingMode READ voltageScalingMode WRITE setVoltageScalingMode
                       NOTIFY voltageScalingChanged)
    Q_PROPERTY(ScalingMode currentScalingMode READ currentScalingMode WRITE setCurrentScalingMode
                       NOTIFY currentScalingChanged)
    Q_PROPERTY(qreal voltageCutoff READ voltageCutoff WRITE setVoltageCutoff NOTIFY
                       voltageScalingChanged)
    Q_PROPERTY(qreal currentCutoff READ currentCutoff WRITE setCurrentCutoff NOTIFY
                       currentScalingChanged)

    // Computed properties for convenience
    Q_PROPERTY(qreal maxVisibleVoltage READ maxVisibleVoltage NOTIFY voltageScalingChanged)
    Q_PROPERTY(qreal maxVisibleCurrent READ maxVisibleCurrent NOTIFY currentScalingChanged)

    explicit ViewStateModel(QObject *parent = nullptr);

    // Property getters
    inline MagnitudeMode magnitudeMode() const { return m_magnitudeMode; }
    inline int phaseReferenceSignalIndex() const { return m_phaseReferenceSignalIndex; }
    inline bool isPausedLocal() const { return m_isPausedLocal; }
    inline QList<bool> signalVisibility() const { return m_signalVisibility; }
    inline ScalingMode voltageScalingMode() const { return m_voltageScalingMode; }
    inline ScalingMode currentScalingMode() const { return m_currentScalingMode; }
    inline qreal voltageCutoff() const { return m_voltageCutoff; }
    inline qreal currentCutoff() const { return m_currentCutoff; }
    inline qreal maxVisibleVoltage() const { return m_voltageCutoff; }
    inline qreal maxVisibleCurrent() const { return m_currentCutoff; }

    // Property setters
    void setMagnitudeMode(MagnitudeMode mode);
    void setPhaseReferenceSignalIndex(int index);
    void setIsPausedLocal(bool paused);
    void setVoltageScalingMode(ScalingMode mode);
    void setCurrentScalingMode(ScalingMode mode);
    void setVoltageCutoff(qreal cutoff);
    void setCurrentCutoff(qreal cutoff);

    // Invokable methods for QML
    Q_INVOKABLE void toggleSignalVisibility(int signalIndex);
    Q_INVOKABLE qreal getEffectiveMagnitude(const SignalData *signal) const;
    Q_INVOKABLE qreal getEffectivePhase(const SignalData *signal, int signalIndex) const;
    Q_INVOKABLE bool isSignalVisible(int signalIndex) const;
    Q_INVOKABLE void setSignalDataModel(SignalDataModel *model);

public slots:
    void updateDynamicScaling();

signals:
    void magnitudeModeChanged();
    void phaseReferenceChanged();
    void pauseStateChanged();
    void signalVisibilityChanged();
    void voltageScalingChanged();
    void currentScalingChanged();

private:
    // Default cutoff values for fallback
    static constexpr qreal DEFAULT_VOLTAGE_CUTOFF = 300.0; // 300V
    static constexpr qreal DEFAULT_CURRENT_CUTOFF = 20.0; // 20A

    // Data-related state
    MagnitudeMode m_magnitudeMode = MagnitudeMode::RMS;
    int m_phaseReferenceSignalIndex = -1; // Default to Absolute (no reference)
    bool m_isPausedLocal = false;

    // Plot-related state
    QList<bool> m_signalVisibility; // Initialized to all true
    ScalingMode m_voltageScalingMode = ScalingMode::Dynamic;
    ScalingMode m_currentScalingMode = ScalingMode::Dynamic;
    qreal m_voltageCutoff = DEFAULT_VOLTAGE_CUTOFF;
    qreal m_currentCutoff = DEFAULT_CURRENT_CUTOFF;

    // Reference to signal data model (not owned)
    SignalDataModel *m_signalDataModel = nullptr;

    // Helper methods
    void initializeVisibility();
    qreal calculateMaxMagnitude(const QString &signalType) const;
};

#endif // VIEW_STATE_MODEL_H
