#include "viewstatemodel.h"
#include <QtMath>

ViewStateModel::ViewStateModel(QObject *parent)
    : QObject(parent)
{
    // Initialize all signals as visible by default
    initializeVisibility();
}

void ViewStateModel::initializeVisibility()
{
    // Initialize with 6 signals (matching qpmu::N_Channels)
    m_signalVisibility.clear();
    for (int i = 0; i < 6; ++i) {
        m_signalVisibility.append(true);
    }
}

void ViewStateModel::setSignalDataModel(SignalDataModel* model)
{
    m_signalDataModel = model;

    // Connect to data updates for dynamic scaling
    if (m_signalDataModel) {
        connect(m_signalDataModel, &QAbstractItemModel::dataChanged,
                this, &ViewStateModel::updateDynamicScaling);
    }
}

// Property setters

void ViewStateModel::setMagnitudeMode(MagnitudeMode mode)
{
    if (m_magnitudeMode != mode) {
        m_magnitudeMode = mode;
        emit magnitudeModeChanged();
    }
}

void ViewStateModel::setPhaseReferenceSignalIndex(int index)
{
    // Index range: -1 (absolute) or 0-5 (signal index)
    if (index < -1 || index >= m_signalVisibility.size()) {
        qWarning() << "Invalid phase reference signal index:" << index;
        return;
    }

    if (m_phaseReferenceSignalIndex != index) {
        m_phaseReferenceSignalIndex = index;
        emit phaseReferenceChanged();
    }
}

void ViewStateModel::setIsPausedLocal(bool paused)
{
    if (m_isPausedLocal != paused) {
        m_isPausedLocal = paused;
        emit pauseStateChanged();
    }
}

void ViewStateModel::setVoltageScalingMode(ScalingMode mode)
{
    if (m_voltageScalingMode != mode) {
        m_voltageScalingMode = mode;
        emit voltageScalingChanged();

        // Immediately update if switching to dynamic mode
        if (mode == ScalingMode::Dynamic) {
            updateDynamicScaling();
        }
    }
}

void ViewStateModel::setCurrentScalingMode(ScalingMode mode)
{
    if (m_currentScalingMode != mode) {
        m_currentScalingMode = mode;
        emit currentScalingChanged();

        // Immediately update if switching to dynamic mode
        if (mode == ScalingMode::Dynamic) {
            updateDynamicScaling();
        }
    }
}

void ViewStateModel::setVoltageCutoff(qreal cutoff)
{
    if (!qFuzzyCompare(m_voltageCutoff, cutoff)) {
        m_voltageCutoff = cutoff;
        emit voltageScalingChanged();
    }
}

void ViewStateModel::setCurrentCutoff(qreal cutoff)
{
    if (!qFuzzyCompare(m_currentCutoff, cutoff)) {
        m_currentCutoff = cutoff;
        emit currentScalingChanged();
    }
}

// Invokable methods

void ViewStateModel::toggleSignalVisibility(int signalIndex)
{
    if (signalIndex < 0 || signalIndex >= m_signalVisibility.size()) {
        qWarning() << "Invalid signal index for visibility toggle:" << signalIndex;
        return;
    }

    m_signalVisibility[signalIndex] = !m_signalVisibility[signalIndex];
    emit signalVisibilityChanged();
}

qreal ViewStateModel::getEffectiveMagnitude(const SignalData* signal) const
{
    if (!signal) {
        return 0.0;
    }

    qreal magnitude = signal->magnitude();

    if (m_magnitudeMode == MagnitudeMode::Peak) {
        // Convert RMS to Peak: Peak = RMS × √2
        return magnitude * M_SQRT2;
    }

    // RMS mode - return as-is
    return magnitude;
}

qreal ViewStateModel::getEffectivePhase(const SignalData* signal, int signalIndex) const
{
    if (!signal || !m_signalDataModel) {
        return 0.0;
    }

    qreal rawPhase = signal->phase();

    // Absolute reference - no adjustment
    if (m_phaseReferenceSignalIndex < 0) {
        return rawPhase;
    }

    // Signal is its own reference - always 0°
    if (signalIndex == m_phaseReferenceSignalIndex) {
        return 0.0;
    }

    // Get reference signal phase
    QModelIndex refIndex = m_signalDataModel->index(m_phaseReferenceSignalIndex, 0);
    QVariant refVariant = m_signalDataModel->data(refIndex, SignalDataModel::SignalDataRole);

    if (!refVariant.isValid()) {
        qWarning() << "Could not get reference signal data for index:" << m_phaseReferenceSignalIndex;
        return rawPhase;
    }

    SignalData* refSignal = refVariant.value<SignalData*>();
    if (!refSignal) {
        return rawPhase;
    }

    qreal refPhase = refSignal->phase();

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

bool ViewStateModel::isSignalVisible(int signalIndex) const
{
    if (signalIndex < 0 || signalIndex >= m_signalVisibility.size()) {
        return false;
    }

    return m_signalVisibility[signalIndex];
}

// Dynamic scaling

void ViewStateModel::updateDynamicScaling()
{
    if (!m_signalDataModel) {
        return;
    }

    // Update voltage cutoff if in dynamic mode
    if (m_voltageScalingMode == ScalingMode::Dynamic) {
        qreal maxVoltage = calculateMaxMagnitude("Voltage");

        if (maxVoltage > 0.0) {
            // Add 10% padding
            m_voltageCutoff = maxVoltage * 1.1;
        } else {
            // Fallback to default
            m_voltageCutoff = DEFAULT_VOLTAGE_CUTOFF;
        }

        emit voltageScalingChanged();
    }

    // Update current cutoff if in dynamic mode
    if (m_currentScalingMode == ScalingMode::Dynamic) {
        qreal maxCurrent = calculateMaxMagnitude("Current");

        if (maxCurrent > 0.0) {
            // Add 10% padding
            m_currentCutoff = maxCurrent * 1.1;
        } else {
            // Fallback to default
            m_currentCutoff = DEFAULT_CURRENT_CUTOFF;
        }

        emit currentScalingChanged();
    }
}

qreal ViewStateModel::calculateMaxMagnitude(const QString& signalType) const
{
    if (!m_signalDataModel) {
        return 0.0;
    }

    qreal maxMag = 0.0;
    int signalCount = m_signalDataModel->rowCount();

    // Scan ALL signals (visible or not) to find max magnitude for the signal type
    for (int i = 0; i < signalCount; ++i) {
        QModelIndex index = m_signalDataModel->index(i, 0);
        QVariant variant = m_signalDataModel->data(index, SignalDataModel::SignalDataRole);

        if (!variant.isValid()) {
            continue;
        }

        SignalData* signal = variant.value<SignalData*>();
        if (!signal) {
            continue;
        }

        // Check signal type
        if (signal->signalType() == signalType) {
            maxMag = qMax(maxMag, signal->magnitude());
        }
    }

    return maxMag;
}
