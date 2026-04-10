#pragma once

#include <functional>

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QColor>
#include <QObject>

#include "qpmu/core.h"

class SignalDataModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum MagnitudeMode {
        RMS = 0,
        Peak = 1
    };
    Q_ENUM(MagnitudeMode)

    enum ScalingMode {
        Dynamic = 0,
        Manual = 1
    };
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
        struct Payload
        {
            qreal sampleValue = 0.0;
            qreal magnitude = 0.0;
            qreal phaseAngle = 0.0;
            qreal frequency = 0.0;
            qreal rocof = 0.0;
            qreal realPower = 0.0;
            qreal reactivePower = 0.0;
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

    explicit SignalDataModel(QObject *parent = nullptr);

    // Selection model
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
    static constexpr qreal DEFAULT_VOLTAGE_CUTOFF = 300.0;
    static constexpr qreal DEFAULT_CURRENT_CUTOFF = 20.0;

    QItemSelectionModel *m_selectionModel;

    MagnitudeMode m_magnitudeMode = MagnitudeMode::RMS;
    int m_phaseReferenceIndex = -1;
    bool m_isPaused = false;

    ScalingMode m_voltageScalingMode = ScalingMode::Dynamic;
    ScalingMode m_currentScalingMode = ScalingMode::Dynamic;
    qreal m_voltageCutoff = DEFAULT_VOLTAGE_CUTOFF;
    qreal m_currentCutoff = DEFAULT_CURRENT_CUTOFF;

    qreal getEffectiveMagnitude(const SignalData &signal) const;
    qreal getEffectivePhase(const SignalData &signal, int signalIndex) const;
    qreal calculateMaxMagnitude(const QString &signalType) const;
    void computePower();
    void updateComputedDisplayValues(bool emitSignal = true);

    QList<SignalData> m_signals;

    static const QColor s_colorPalette[qpmu::Signal_Infos.size()];
    static const QList<TableColumnMeta> s_tableColumnMeta;
};
