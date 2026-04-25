#include "signaldatamodel.h"
#include "theme.h"

#include <QtMath>
#include <cmath>

// ── Construction ────────────────────────────────────────────────────────────

SignalDataModel::SignalDataModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_signals.reserve(SIGNAL_COUNT);
    for (int i = 0; i < SIGNAL_COUNT; ++i) {
        SignalData sd;
        sd.info = qpmu::Signal_Infos[i];
        sd.color = Theme::Colors::signal[i];
        m_signals.append(sd);
    }

    m_selectionModel = new QItemSelectionModel(this, this);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &selected, const QItemSelection &deselected) {
                for (const auto &idx : selected.indexes())
                    emit dataChanged(idx, idx, { Qt::ToolTipRole });
                for (const auto &idx : deselected.indexes())
                    emit dataChanged(idx, idx, { Qt::ToolTipRole });
            });
}

// ── QAbstractItemModel interface ────────────────────────────────────────────

QHash<int, QByteArray> SignalDataModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { Qt::DisplayRole, "display" },     { Qt::DecorationRole, "decoration" },
        { Qt::ToolTipRole, "toolTip" },     { NameRole, "name" },
        { TypeSymbolRole, "typeSymbol" },   { UnitSymbolRole, "unitSymbol" },
        { PhaseSymbolRole, "phaseSymbol" }, { SampleValueRole, "sampleValue" },
        { MagnitudeRole, "magnitude" },     { PhaseAngleRole, "phaseAngle" },
        { FrequencyRole, "frequency" },     { RocofRole, "rocof" },
        { RealPowerRole, "realPower" },     { ReactivePowerRole, "reactivePower" },
    };
    return roles;
}

QModelIndex SignalDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return {};
    return createIndex(row, column);
}

QModelIndex SignalDataModel::parent(const QModelIndex &) const
{
    return {};
}

int SignalDataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_signals.count();
}

int SignalDataModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant SignalDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const auto &sig = m_signals.at(index.row());
    qreal mag = effectiveMagnitude(sig);
    qreal phase = effectivePhase(sig, index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return QString::number(mag, 'f', 2) + " " + sig.info.unit_symbol;
        case 1:
            return QString::number(phase, 'f', 1) + QStringLiteral("°");
        case 2:
            return QString::number(sig.payload.realPower, 'f', 1) + " W";
        case 3:
            return QString::number(sig.payload.reactivePower, 'f', 1) + " VAR";
        default:
            return {};
        }
    case Qt::DecorationRole:
        return sig.color;
    case Qt::ToolTipRole:
        if (!m_selectionModel->isSelected(index))
            return {};
        return QStringLiteral("%1: %2 %3 ∠ %4°")
                .arg(sig.info.name)
                .arg(mag, 0, 'f', 2)
                .arg(sig.info.unit_symbol)
                .arg(phase, 0, 'f', 1);
    case NameRole:
        return QString(sig.info.name);
    case TypeSymbolRole:
        return QString(QChar(sig.info.type_symbol));
    case UnitSymbolRole:
        return QString(QChar(sig.info.unit_symbol));
    case PhaseSymbolRole:
        return QString(QChar(sig.info.phase_symbol));
    case SampleValueRole:
        return sig.payload.sampleValue;
    case MagnitudeRole:
        return mag;
    case PhaseAngleRole:
        return phase;
    case FrequencyRole:
        return sig.payload.frequency;
    case RocofRole:
        return sig.payload.rocof;
    case RealPowerRole:
        return sig.payload.realPower;
    case ReactivePowerRole:
        return sig.payload.reactivePower;
    default:
        return {};
    }
}

QVariant SignalDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Horizontal)
        return QString(s_columnHeaders[section]);
    return QString(m_signals[section].info.name);
}

bool SignalDataModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

Qt::ItemFlags SignalDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

// ── Property getters ────────────────────────────────────────────────────────

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

// ── Property setters ────────────────────────────────────────────────────────

void SignalDataModel::setMagnitudeMode(MagnitudeMode mode)
{
    if (m_magnitudeMode != mode) {
        m_magnitudeMode = mode;
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
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
        emit dataChanged(this->index(0, 0), this->index(rowCount() - 1, columnCount() - 1));
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
        if (mode == Dynamic)
            updateDynamicScaling();
        emit voltageScalingChanged();
    }
}

void SignalDataModel::setCurrentScalingMode(ScalingMode mode)
{
    if (m_currentScalingMode != mode) {
        m_currentScalingMode = mode;
        if (mode == Dynamic)
            updateDynamicScaling();
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

// ── Data updates ────────────────────────────────────────────────────────────

void SignalDataModel::updateFromFrame(const qpmu::Measurement_Frame &frame)
{
    if (m_isPaused)
        return;

    for (int i = 0; i < SIGNAL_COUNT; ++i) {
        const auto &est = frame.estimate_array[i];
        auto &p = m_signals[i].payload;
        p.magnitude = std::abs(est.phasor);
        p.phaseAngle = std::arg(est.phasor) * 180.0 / M_PI;
        p.frequency = est.frequency;
        p.rocof = est.rocof;
    }

    computePower();
    updateDynamicScaling();
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void SignalDataModel::updateSimulatedData(qreal simulationTime)
{
    if (m_isPaused)
        return;

    qreal vMag = 120.0 + 5.0 * qSin(simulationTime * 0.5);
    qreal iMag = 10.0 + 1.0 * qSin(simulationTime * 0.7);
    qreal freq = 60.0 + 0.05 * qSin(simulationTime * 0.3);

    for (int i = 0; i < SIGNAL_COUNT; ++i) {
        qreal phaseDeg = (i < 3) ? (i * 120.0) // voltages: 0°, 120°, 240°
                                 : ((i - 3) * 120.0 - 30.0); // currents: lagging by 30°
        qreal mag = (i < 3) ? vMag : iMag;

        auto &p = m_signals[i].payload;
        p.magnitude = mag;
        p.phaseAngle = phaseDeg;
        p.frequency = freq;
        p.rocof = 0;
    }

    computePower();
    updateDynamicScaling();
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

// ── Internal helpers ────────────────────────────────────────────────────────

qreal SignalDataModel::effectiveMagnitude(const SignalData &sig) const
{
    return (m_magnitudeMode == Peak) ? sig.payload.magnitude * M_SQRT2 : sig.payload.magnitude;
}

qreal SignalDataModel::effectivePhase(const SignalData &sig, int signalIndex) const
{
    if (m_phaseReferenceIndex < 0)
        return sig.payload.phaseAngle;
    if (signalIndex == m_phaseReferenceIndex)
        return 0.0;
    qreal rel = sig.payload.phaseAngle - m_signals[m_phaseReferenceIndex].payload.phaseAngle;
    return std::remainder(rel, 360.0);
}

void SignalDataModel::updateDynamicScaling()
{
    bool changed = false;

    if (m_voltageScalingMode == Dynamic) {
        qreal maxV = maxMagnitude(qpmu::Signal_Info::Type_Voltage);
        qreal cutoff = maxV > 0 ? maxV * 1.1 : DEFAULT_VOLTAGE_CUTOFF;
        if (!qFuzzyCompare(m_voltageCutoff, cutoff)) {
            m_voltageCutoff = cutoff;
            changed = true;
        }
    }

    if (m_currentScalingMode == Dynamic) {
        qreal maxI = maxMagnitude(qpmu::Signal_Info::Type_Current);
        qreal cutoff = maxI > 0 ? maxI * 1.1 : DEFAULT_CURRENT_CUTOFF;
        if (!qFuzzyCompare(m_currentCutoff, cutoff)) {
            m_currentCutoff = cutoff;
            changed = true;
        }
    }

    if (changed) {
        emit voltageScalingChanged();
        emit currentScalingChanged();
    }
}

qreal SignalDataModel::maxMagnitude(qpmu::Signal_Info::Type_ID type) const
{
    qreal max = 0;
    for (const auto &sig : m_signals) {
        if (sig.info.type_id == type)
            max = qMax(max, sig.payload.magnitude);
    }
    return max;
}

void SignalDataModel::computePower()
{
    // Pair voltage/current per phase: VA↔IA (0↔3), VB↔IB (1↔4), VC↔IC (2↔5)
    for (int ph = 0; ph < 3; ++ph) {
        auto &v = m_signals[ph].payload;
        auto &i = m_signals[ph + 3].payload;
        qreal diffRad = (v.phaseAngle - i.phaseAngle) * M_PI / 180.0;
        qreal apparent = v.magnitude * i.magnitude;
        qreal real = apparent * qCos(diffRad);
        qreal reactive = apparent * qSin(diffRad);
        v.realPower = i.realPower = real;
        v.reactivePower = i.reactivePower = reactive;
    }
}
