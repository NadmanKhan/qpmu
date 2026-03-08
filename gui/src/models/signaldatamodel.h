#ifndef SIGNAL_DATA_MODEL_H
#define SIGNAL_DATA_MODEL_H

#include <functional>

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QColor>
#include <QObject>
#include <QtQmlIntegration>

#include "qpmu/core.h"

// Item model managing all signal data
// Exposes collection of Signal objects as QAbstractItemModel for QML
// Can be used with TableView, ListView, and custom graph views
class SignalDataModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    // Display modes
    enum MagnitudeMode {
        RMS = 0, // Display RMS magnitude (as-is from estimator)
        Peak = 1 // Display peak magnitude (RMS × √2)
    };
    Q_ENUM(MagnitudeMode)

    enum ScalingMode {
        Dynamic = 0, // Auto-scale based on signal magnitudes
        Manual = 1 // User-defined fixed scale
    };
    Q_ENUM(ScalingMode)

    enum SignalDataRoles {
        // Used from standard Qt roles:
        // Qt::DisplayRole for formatted value display
        // Qt::DecorationRole for signal color
        // Qt::ToolTipRole for detailed info tooltip

        // Info roles:
        NameRole = Qt::UserRole + 1,
        TypeSymbolRole,
        UnitSymbolRole,
        PhaseSymbolRole,

        // Payload roles:
        SampleValueRole,
        MagnitudeRole,
        PhaseAngleRole,
        FrequencyRole,
        RocofRole,
        RealPowerRole,
        ReactivePowerRole,
    };

    Q_ENUM(SignalDataRoles)

    struct SignalData
    {
        qpmu::Signal_Info info = {};
        struct Payload
        {
            qreal sampleValue = 0.0;
            qreal magnitude = 0.0; // Volts or Amperes - absolute value of phasor estimate
            qreal phaseAngle = 0.0; // Degrees - angle of phasor estimate
            qreal frequency = 0.0; // Hz
            qreal rocof = 0.0; // Hz/s
            qreal realPower = 0.0; // W
            qreal reactivePower = 0.0; // VAR
        } payload;
        struct Settings
        {
            QColor color;
        } settings;
    };

    struct TableColumnMeta
    {
        QString header;
        std::function<QString(const SignalData &signal)> formatValue;
    };

    // Selection model for tracking selected signals
    Q_PROPERTY(QItemSelectionModel *selectionModel READ selectionModel CONSTANT)

    // Properties for display settings
    Q_PROPERTY(MagnitudeMode magnitudeMode READ magnitudeMode WRITE setMagnitudeMode NOTIFY
                       magnitudeModeChanged)
    Q_PROPERTY(int phaseReferenceIndex READ phaseReferenceIndex WRITE setPhaseReferenceIndex NOTIFY
                       phaseReferenceChanged)
    Q_PROPERTY(bool isPaused READ isPaused WRITE setIsPaused NOTIFY pauseStateChanged)

    // Properties for plot scaling
    Q_PROPERTY(ScalingMode voltageScalingMode READ voltageScalingMode WRITE setVoltageScalingMode
                       NOTIFY voltageScalingChanged)
    Q_PROPERTY(ScalingMode currentScalingMode READ currentScalingMode WRITE setCurrentScalingMode
                       NOTIFY currentScalingChanged)
    Q_PROPERTY(qreal voltageCutoff READ voltageCutoff WRITE setVoltageCutoff NOTIFY
                       voltageScalingChanged)
    Q_PROPERTY(qreal currentCutoff READ currentCutoff WRITE setCurrentCutoff NOTIFY
                       currentScalingChanged)

    explicit SignalDataModel(QObject *parent = nullptr);

    // Selection model getter
    QItemSelectionModel *selectionModel() const;

    // Property getters
    MagnitudeMode magnitudeMode() const;
    int phaseReferenceIndex() const;
    bool isPaused() const;
    ScalingMode voltageScalingMode() const;
    ScalingMode currentScalingMode() const;
    qreal voltageCutoff() const;
    qreal currentCutoff() const;

    // Property setters
    void setMagnitudeMode(MagnitudeMode mode);
    void setPhaseReferenceIndex(int index);
    void setIsPaused(bool paused);
    void setVoltageScalingMode(ScalingMode mode);
    void setCurrentScalingMode(ScalingMode mode);
    void setVoltageCutoff(qreal cutoff);
    void setCurrentCutoff(qreal cutoff);

    // QAbstractItemModel interface
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Update methods
    void updateFromFrame(const qpmu::Measurement_Frame &frame);
    void updateSimulatedData(qreal simulationTime);

signals:
    void magnitudeModeChanged();
    void phaseReferenceChanged();
    void pauseStateChanged();
    void voltageScalingChanged();
    void currentScalingChanged();

public slots:
    void updateDynamicScaling();

private:
    // Default cutoff values
    static constexpr qreal DEFAULT_VOLTAGE_CUTOFF = 300.0; // 300V
    static constexpr qreal DEFAULT_CURRENT_CUTOFF = 20.0; // 20A

    // Selection model
    QItemSelectionModel *m_selectionModel;

    // Display settings
    MagnitudeMode m_magnitudeMode = MagnitudeMode::RMS;
    int m_phaseReferenceIndex = -1; // -1 = absolute, 0-5 = signal index
    bool m_isPaused = false;

    // Plot scaling settings
    ScalingMode m_voltageScalingMode = ScalingMode::Dynamic;
    ScalingMode m_currentScalingMode = ScalingMode::Dynamic;
    qreal m_voltageCutoff = DEFAULT_VOLTAGE_CUTOFF;
    qreal m_currentCutoff = DEFAULT_CURRENT_CUTOFF;

    // Helper methods
    qreal getEffectiveMagnitude(const SignalData &signal) const;
    qreal getEffectivePhase(const SignalData &signal, int signalIndex) const;
    qreal calculateMaxMagnitude(const QString &signalType) const;
    void computePower();
    void updateComputedDisplayValues();

    // Signal data array
    QList<SignalData> m_signals;

    // Color palette for signals
    static const QColor s_colorPalette[qpmu::Signal_Infos.size()];

    static const QList<TableColumnMeta> s_tableColumnMeta;
};

#endif // SIGNAL_DATA_MODEL_H
