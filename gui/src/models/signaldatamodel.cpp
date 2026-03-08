#include <QtMath>
#include <QSize>

#include "signaldatamodel.h"

const QColor SignalDataModel::s_colorPalette[qpmu::Signal_Infos.size()] = {
    QColor("#ff6b6b"), // VA - Coral red
    QColor("#4ecdc4"), // VB - Teal
    QColor("#45b7d1"), // VC - Sky blue
    QColor("#ffd93d"), // IA - Golden yellow
    QColor("#c56cf0"), // IB - Purple
    QColor("#95e1d3") // IC - Mint
};

const QList<SignalDataModel::TableColumnMeta> SignalDataModel::s_tableColumnMeta = {
    {
            "Magnitude",
            [](const SignalData &s) {
                // Magnitude formatting - actual value computed in data() method
                return QString::number(s.payload.magnitude, 'f', 2) + " " + s.info.unit_symbol;
            },
    },
    {
            "Phase",
            [](const SignalData &s) {
                // Phase formatting - actual value computed in data() method
                return QString::number(s.payload.phaseAngle, 'f', 1) + "°";
            },
    },
    {
            "Real Power",
            [](const SignalData &s) { return QString::number(s.payload.realPower, 'f', 1) + " W"; },
    },
    {
            "Reactive Power",
            [](const SignalData &s) {
                return QString::number(s.payload.reactivePower, 'f', 1) + " VAR";
            },
    }
};

SignalDataModel::SignalDataModel(QObject *parent) : QAbstractItemModel(parent)
{
    // Create signal data from core Signal_Info array
    m_signals.reserve(qpmu::Signal_Infos.size());

    for (std::size_t i = 0; i < qpmu::Signal_Infos.size(); ++i) {
        SignalData signalData;
        signalData.info = qpmu::Signal_Infos[i];
        signalData.payload = {};
        signalData.settings.color = s_colorPalette[i];
        m_signals.append(signalData);
    }

    // Create selection model (no signals selected by default - selection controls tooltip
    // visibility)
    m_selectionModel = new QItemSelectionModel(this, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &selected, const QItemSelection &deselected) {
                for (const auto &index : selected.indexes()) {
                    emit dataChanged(index, index, { Qt::ToolTipRole });
                }
                for (const auto &index : deselected.indexes()) {
                    emit dataChanged(index, index, { Qt::ToolTipRole });
                }
            });
}

// ====================================================================================================
// Overrides
// ====================================================================================================

QHash<int, QByteArray> SignalDataModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    // Standard Qt roles
    roles[Qt::DisplayRole] = "display";
    roles[Qt::DecorationRole] = "decoration";
    roles[Qt::ToolTipRole] = "toolTip";
    // Custom roles for signal info
    roles[NameRole] = "name";
    roles[TypeSymbolRole] = "typeSymbol";
    roles[UnitSymbolRole] = "unitSymbol";
    roles[PhaseSymbolRole] = "phaseSymbol";
    // Custom roles for signal payload
    roles[SampleValueRole] = "sampleValue";
    roles[MagnitudeRole] = "magnitude";
    roles[PhaseAngleRole] = "phaseAngle";
    roles[FrequencyRole] = "frequency";
    roles[RocofRole] = "rocof";
    roles[RealPowerRole] = "realPower";
    roles[ReactivePowerRole] = "reactivePower";
    return roles;
}

QModelIndex SignalDataModel::index(int row, int column, const QModelIndex &parent) const
{
    // For a flat list model, only the root level (having invalid parent) has valid
    // indices, so return invalid index for any valid parent
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex SignalDataModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex(); // Flat model, no parent
}

int SignalDataModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_signals.count();
}

int SignalDataModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return SignalDataModel::s_tableColumnMeta.size();
}

QVariant SignalDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto &signal = m_signals.at(index.row());

    // Compute effective values based on display settings
    qreal effectiveMagnitude = getEffectiveMagnitude(signal);
    qreal effectivePhase = getEffectivePhase(signal, index.row());

    switch (role) {
    // Standard Qt roles
    case Qt::DisplayRole:
        // For table display, use effective values
        if (index.column() == 0) {
            // Magnitude column
            return QString::number(effectiveMagnitude, 'f', 2) + " " + signal.info.unit_symbol;
        } else if (index.column() == 1) {
            // Phase column
            return QString::number(effectivePhase, 'f', 1) + "°";
        } else {
            // Other columns use default formatter
            return SignalDataModel::s_tableColumnMeta[index.column()].formatValue(signal);
        }
    case Qt::DecorationRole:
        return signal.settings.color;
    case Qt::ToolTipRole:
        return m_selectionModel->isSelected(index) ? QString("%1: %2 %3 ∠ %4°")
                                                             .arg(signal.info.name)
                                                             .arg(effectiveMagnitude, 0, 'f', 2)
                                                             .arg(signal.info.unit_symbol)
                                                             .arg(effectivePhase, 0, 'f', 1)
                                                   : QString(); // Empty tooltip if not selected
    // Custom roles for signal info
    case NameRole:
        return QString(signal.info.name);
    case TypeSymbolRole:
        return QString(signal.info.type_symbol);
    case UnitSymbolRole:
        return QString(signal.info.unit_symbol);
    case PhaseSymbolRole:
        return QString(signal.info.phase_symbol);
    // Custom roles for signal data - return EFFECTIVE values
    case SampleValueRole:
        return signal.payload.sampleValue;
    case MagnitudeRole:
        return effectiveMagnitude; // Use effective magnitude
    case PhaseAngleRole:
        return effectivePhase; // Use effective phase
    case FrequencyRole:
        return signal.payload.frequency;
    case RocofRole:
        return signal.payload.rocof;
    case RealPowerRole:
        return signal.payload.realPower;
    case ReactivePowerRole:
        return signal.payload.reactivePower;
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant SignalDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return SignalDataModel::s_tableColumnMeta[section].header;
        }

    } else /* if (orientation == Qt::Vertical) */ {
        if (role == Qt::DisplayRole) {
            return m_signals[section].info.name;
        }
    }

    return QVariant();
}

bool SignalDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)

    // No editable data in this model - selection is handled by QItemSelectionModel
    return false;
}

Qt::ItemFlags SignalDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void SignalDataModel::computePower()
{
    // Compute power for each phase by pairing voltage and current
    // Phase A: indices 0 (VA) and 3 (IA)
    // Phase B: indices 1 (VB) and 4 (IB)
    // Phase C: indices 2 (VC) and 5 (IC)

    for (int phase = 0; phase < 3; ++phase) {
        int vIndex = phase; // Voltage index: 0, 1, 2
        int iIndex = phase + 3; // Current index: 3, 4, 5

        qreal vMag = m_signals[vIndex].payload.magnitude;
        qreal vPhase = m_signals[vIndex].payload.phaseAngle;
        qreal iMag = m_signals[iIndex].payload.magnitude;
        qreal iPhase = m_signals[iIndex].payload.phaseAngle;

        // Phase difference between voltage and current (in degrees)
        qreal phaseDiff = vPhase - iPhase;
        qreal phaseDiffRad = phaseDiff * M_PI / 180.0;

        // Compute power: P = V * I * cos(θ), Q = V * I * sin(θ)
        qreal apparentPower = vMag * iMag;
        qreal realPower = apparentPower * qCos(phaseDiffRad);
        qreal reactivePower = apparentPower * qSin(phaseDiffRad);

        // Update both voltage and current signals with the same power values
        m_signals[vIndex].payload.realPower = realPower;
        m_signals[vIndex].payload.reactivePower = reactivePower;
        m_signals[iIndex].payload.realPower = realPower;
        m_signals[iIndex].payload.reactivePower = reactivePower;
    }
}

// ====================================================================================================
// Property getters
// ====================================================================================================

QItemSelectionModel *SignalDataModel::selectionModel() const
{
    return m_selectionModel;
}

SignalDataModel::MagnitudeMode SignalDataModel::magnitudeMode() const
{
    return m_magnitudeMode;
}

int SignalDataModel::phaseReferenceIndex() const
{
    return m_phaseReferenceIndex;
}

bool SignalDataModel::isPaused() const
{
    return m_isPaused;
}

SignalDataModel::ScalingMode SignalDataModel::voltageScalingMode() const
{
    return m_voltageScalingMode;
}

SignalDataModel::ScalingMode SignalDataModel::currentScalingMode() const
{
    return m_currentScalingMode;
}

qreal SignalDataModel::voltageCutoff() const
{
    return m_voltageCutoff;
}

qreal SignalDataModel::currentCutoff() const
{
    return m_currentCutoff;
}

// ====================================================================================================
// Property setters
// ====================================================================================================

void SignalDataModel::setMagnitudeMode(MagnitudeMode mode)
{
    if (m_magnitudeMode != mode) {
        m_magnitudeMode = mode;
        updateComputedDisplayValues();
        emit magnitudeModeChanged();
    }
}

void SignalDataModel::setPhaseReferenceIndex(int index)
{
    if (index < -1 || index >= m_signals.size()) {
        qWarning() << "Invalid phase reference index:" << index;
        return;
    }

    if (m_phaseReferenceIndex != index) {
        m_phaseReferenceIndex = index;
        updateComputedDisplayValues();
        emit phaseReferenceChanged();
    }
}

void SignalDataModel::setIsPaused(bool paused)
{
    if (m_isPaused != paused) {
        m_isPaused = paused;
        emit pauseStateChanged();
    }
}

void SignalDataModel::setVoltageScalingMode(ScalingMode mode)
{
    if (m_voltageScalingMode != mode) {
        m_voltageScalingMode = mode;
        if (mode == ScalingMode::Dynamic) {
            updateDynamicScaling();
        }
        emit voltageScalingChanged();
    }
}

void SignalDataModel::setCurrentScalingMode(ScalingMode mode)
{
    if (m_currentScalingMode != mode) {
        m_currentScalingMode = mode;
        if (mode == ScalingMode::Dynamic) {
            updateDynamicScaling();
        }
        emit currentScalingChanged();
    }
}

void SignalDataModel::setVoltageCutoff(qreal cutoff)
{
    if (!qFuzzyCompare(m_voltageCutoff, cutoff)) {
        m_voltageCutoff = cutoff;
        emit voltageScalingChanged();
    }
}

void SignalDataModel::setCurrentCutoff(qreal cutoff)
{
    if (!qFuzzyCompare(m_currentCutoff, cutoff)) {
        m_currentCutoff = cutoff;
        emit currentScalingChanged();
    }
}

// ====================================================================================================
// Update methods
// ====================================================================================================

void SignalDataModel::updateFromFrame(const qpmu::Measurement_Frame &frame)
{
    if (m_isPaused) {
        return;
    }

    // Update each signal from the measurement frame
    for (std::size_t i = 0; i < qpmu::Signal_Infos.size(); ++i) {
        const auto &estimate = frame.estimate_vector[i];
        m_signals[i].payload.magnitude = std::abs(estimate.phasor);
        m_signals[i].payload.phaseAngle =
                std::arg(estimate.phasor) * 180.0 / M_PI; // Convert to degrees
        m_signals[i].payload.frequency = estimate.frequency;
        m_signals[i].payload.rocof = estimate.rocof;
    }

    // Compute power for each phase
    computePower();

    // Update dynamic scaling if enabled
    updateDynamicScaling();

    // Update computed display values (effective magnitude/phase)
    updateComputedDisplayValues();

    // Notify views that data has changed (all rows, all columns)
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void SignalDataModel::updateSimulatedData(qreal simulationTime)
{
    if (m_isPaused) {
        return;
    }

    // Simulate 3-phase voltage and current phasors
    qreal vMag = 120.0 + 5.0 * qSin(simulationTime * 0.5);
    qreal iMag = 10.0 + 1.0 * qSin(simulationTime * 0.7);
    qreal freqVariation = 60.0 + 0.05 * qSin(simulationTime * 0.3);

    // Update each signal
    for (std::size_t i = 0; i < qpmu::Signal_Infos.size(); ++i) {
        qpmu::Estimate estimate;

        if (i < 3) {
            // Voltages: 120V magnitude, 120° apart
            qreal phaseRad = (i * 120.0) * M_PI / 180.0;
            estimate.phasor =
                    std::complex<float>(vMag * std::cos(phaseRad), vMag * std::sin(phaseRad));
            estimate.frequency = freqVariation;
        } else {
            // Currents: 10A magnitude, 120° apart, lagging voltage by 30°
            qreal phaseRad = ((i - 3) * 120.0 - 30.0) * M_PI / 180.0;
            estimate.phasor =
                    std::complex<float>(iMag * std::cos(phaseRad), iMag * std::sin(phaseRad));
            estimate.frequency = freqVariation;
        }
        estimate.rocof = 0.0;

        m_signals[i].payload.magnitude = std::abs(estimate.phasor);
        m_signals[i].payload.phaseAngle =
                std::arg(estimate.phasor) * 180.0 / M_PI; // Convert to degrees
        m_signals[i].payload.frequency = estimate.frequency;
        m_signals[i].payload.rocof = estimate.rocof;
    }

    // Compute power for each phase
    computePower();

    // Update dynamic scaling if enabled
    updateDynamicScaling();

    // Update computed display values (effective magnitude/phase)
    updateComputedDisplayValues();

    // Notify views that data has changed (all rows, all columns)
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

// ====================================================================================================
// Helper methods
// ====================================================================================================

qreal SignalDataModel::getEffectiveMagnitude(const SignalData &signal) const
{
    qreal magnitude = signal.payload.magnitude;

    if (m_magnitudeMode == MagnitudeMode::Peak) {
        // Convert RMS to Peak: Peak = RMS × √2
        return magnitude * M_SQRT2;
    }

    // RMS mode - return as-is
    return magnitude;
}

qreal SignalDataModel::getEffectivePhase(const SignalData &signal, int signalIndex) const
{
    qreal rawPhase = signal.payload.phaseAngle;

    // Absolute reference - no adjustment
    if (m_phaseReferenceIndex < 0) {
        return rawPhase;
    }

    // Signal is its own reference - always 0°
    if (signalIndex == m_phaseReferenceIndex) {
        return 0.0;
    }

    // Get reference signal phase
    if (m_phaseReferenceIndex >= m_signals.size()) {
        qWarning() << "Invalid phase reference index:" << m_phaseReferenceIndex;
        return rawPhase;
    }

    qreal refPhase = m_signals[m_phaseReferenceIndex].payload.phaseAngle;

    // Calculate relative phase
    qreal relativePhase = rawPhase - refPhase;

    // Normalize to [-180°, 180°] range
    while (relativePhase > 180.0) {
        relativePhase -= 360.0;
    }
    while (relativePhase < -180.0) {
        relativePhase += 360.0;
    }

    return relativePhase;
}

void SignalDataModel::updateDynamicScaling()
{
    bool changed = false;

    // Update voltage cutoff if in dynamic mode
    if (m_voltageScalingMode == ScalingMode::Dynamic) {
        qreal maxVoltage = calculateMaxMagnitude("V");
        qreal newCutoff = maxVoltage > 0.0 ? maxVoltage * 1.1 : DEFAULT_VOLTAGE_CUTOFF;

        if (!qFuzzyCompare(m_voltageCutoff, newCutoff)) {
            m_voltageCutoff = newCutoff;
            changed = true;
        }
    }

    // Update current cutoff if in dynamic mode
    if (m_currentScalingMode == ScalingMode::Dynamic) {
        qreal maxCurrent = calculateMaxMagnitude("I");
        qreal newCutoff = maxCurrent > 0.0 ? maxCurrent * 1.1 : DEFAULT_CURRENT_CUTOFF;

        if (!qFuzzyCompare(m_currentCutoff, newCutoff)) {
            m_currentCutoff = newCutoff;
            changed = true;
        }
    }

    if (changed) {
        emit voltageScalingChanged();
        emit currentScalingChanged();
    }
}

qreal SignalDataModel::calculateMaxMagnitude(const QString &typeSymbol) const
{
    qreal maxMag = 0.0;

    for (const auto &signal : m_signals) {
        if (signal.info.type_symbol == typeSymbol) {
            maxMag = qMax(maxMag, signal.payload.magnitude);
        }
    }

    return maxMag;
}

void SignalDataModel::updateComputedDisplayValues()
{
    // This updates the effective magnitude and phase for all signals
    // These values are computed on-the-fly in the data() method
    // So we just need to notify that data has changed
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}
