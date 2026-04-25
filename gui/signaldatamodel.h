#pragma once

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QColor>

#include "qpmu/core.h"

/// Flat table model (6 rows × 4 columns) exposing PMU signal data.
/// Rows = signals (VA, VB, VC, IA, IB, IC). Columns = Magnitude, Phase, Real Power, Reactive Power.
/// Custom roles provide per-signal metadata and computed values (effective magnitude/phase
/// account for display settings like RMS/Peak mode and phase reference).
class SignalDataModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum MagnitudeMode { RMS = 0, Peak = 1 };
    Q_ENUM(MagnitudeMode)

    enum ScalingMode { Dynamic = 0, Manual = 1 };
    Q_ENUM(ScalingMode)

    enum SignalDataRoles {
        NameRole = Qt::UserRole + 1,
        TypeSymbolRole,
        UnitSymbolRole,
        PhaseSymbolRole,
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
        struct
        {
            qreal sampleValue = 0;
            qreal magnitude = 0;
            qreal phaseAngle = 0;
            qreal frequency = 0;
            qreal rocof = 0;
            qreal realPower = 0;
            qreal reactivePower = 0;
        } payload;
        QColor color;
    };

    explicit SignalDataModel(QObject *parent = nullptr);

    QItemSelectionModel *selectionModel() const;

    // -- Property getters --
    MagnitudeMode magnitudeMode() const;
    int phaseReferenceIndex() const;
    bool isPaused() const;
    ScalingMode voltageScalingMode() const;
    ScalingMode currentScalingMode() const;
    qreal voltageCutoff() const;
    qreal currentCutoff() const;

    // -- Property setters --
    void setMagnitudeMode(MagnitudeMode mode);
    void setPhaseReferenceIndex(int index);
    void setIsPaused(bool paused);
    void setVoltageScalingMode(ScalingMode mode);
    void setCurrentScalingMode(ScalingMode mode);
    void setVoltageCutoff(qreal cutoff);
    void setCurrentCutoff(qreal cutoff);

    // -- QAbstractItemModel interface --
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

    // -- Data updates --
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
    static constexpr int SIGNAL_COUNT = qpmu::Signal_Infos.size();
    static constexpr qreal DEFAULT_VOLTAGE_CUTOFF = 300.0;
    static constexpr qreal DEFAULT_CURRENT_CUTOFF = 20.0;
    static constexpr const char *s_columnHeaders[] = { "Magnitude", "Phase", "Real Power",
                                                       "Reactive Power" };

    qreal effectiveMagnitude(const SignalData &signal) const;
    qreal effectivePhase(const SignalData &signal, int signalIndex) const;
    qreal maxMagnitude(qpmu::Signal_Info::Type_ID type) const;
    void computePower();

    QItemSelectionModel *m_selectionModel;
    QList<SignalData> m_signals;

    MagnitudeMode m_magnitudeMode = RMS;
    int m_phaseReferenceIndex = -1;
    bool m_isPaused = false;

    ScalingMode m_voltageScalingMode = Dynamic;
    ScalingMode m_currentScalingMode = Dynamic;
    qreal m_voltageCutoff = DEFAULT_VOLTAGE_CUTOFF;
    qreal m_currentCutoff = DEFAULT_CURRENT_CUTOFF;
};
