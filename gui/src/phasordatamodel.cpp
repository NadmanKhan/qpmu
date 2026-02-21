#include "phasordatamodel.h"
#include "qpmu/core.h"
#include <QtMath>
#include <QColor>

PhasorDataModel::PhasorDataModel(QObject *parent) : QObject(parent)
{
    // Initialize with zero data for 6 signals
    for (int i = 0; i < 6; ++i) {
        m_phasorMagnitudes.append(0.0);
        m_phasorPhases.append(0.0);
        m_frequencies.append(60.0); // Default 60 Hz
    }

    m_lastSampleTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // Setup simulation timer for testing
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(100); // 10 Hz update rate
    connect(m_simulationTimer, &QTimer::timeout, this, &PhasorDataModel::updateSimulatedData);
}

QVariantList PhasorDataModel::signalColors() const
{
    QVariantList colors;
    // VA, VB, VC: Refined palette for better visibility
    colors.append(QColor("#ff6b6b"));  // Coral red
    colors.append(QColor("#4ecdc4"));  // Teal
    colors.append(QColor("#45b7d1"));  // Sky blue
    // IA, IB, IC: Complementary colors
    colors.append(QColor("#ffd93d"));  // Golden yellow
    colors.append(QColor("#c56cf0"));  // Purple
    colors.append(QColor("#95e1d3"));  // Mint
    return colors;
}

QVariantList PhasorDataModel::signalNames() const
{
    QVariantList names;
    names << "VA" << "VB" << "VC" << "IA" << "IB" << "IC";
    return names;
}

void PhasorDataModel::togglePause()
{
    m_isPaused = !m_isPaused;
    emit pauseStateChanged();
}

void PhasorDataModel::startSimulation()
{
    m_simulationTimer->start();
}

void PhasorDataModel::updateSimulatedData()
{
    if (m_isPaused) {
        return;
    }

    // Simulate 3-phase voltage and current phasors
    // Voltages: 120V magnitude, 120° apart
    // Currents: 10A magnitude, 120° apart, lagging voltage by 30°

    m_simulationTime += 0.1; // Advance time

    QVariantList magnitudes;
    QVariantList phases;
    QVariantList freqs;

    // Add some variation to make it look live
    qreal vMag = 120.0 + 5.0 * qSin(m_simulationTime * 0.5);
    qreal iMag = 10.0 + 1.0 * qSin(m_simulationTime * 0.7);
    qreal freqVariation = 60.0 + 0.05 * qSin(m_simulationTime * 0.3);

    // Voltages (VA, VB, VC)
    for (int i = 0; i < 3; ++i) {
        magnitudes.append(vMag);
        phases.append(i * 120.0); // 0°, 120°, 240°
        freqs.append(freqVariation);
    }

    // Currents (IA, IB, IC) - lagging by 30°
    for (int i = 0; i < 3; ++i) {
        magnitudes.append(iMag);
        phases.append(i * 120.0 - 30.0); // -30°, 90°, 210°
        freqs.append(freqVariation);
    }

    qreal samplingRate = 1000.0; // 1 kHz
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch() * 1000; // Convert to microseconds

    updateProperties(magnitudes, phases, freqs, samplingRate, timestamp);
}

void PhasorDataModel::updateProperties(const QVariantList &magnitudes, const QVariantList &phases,
                                       const QVariantList &freqs, qreal samplingRate,
                                       qint64 timestamp)
{
    m_phasorMagnitudes = magnitudes;
    m_phasorPhases = phases;
    m_frequencies = freqs;
    m_samplingRate = samplingRate;
    m_lastSampleTime = QDateTime::fromMSecsSinceEpoch(timestamp / 1000).toString("hh:mm:ss.zzz");

    emit dataUpdated();
}

void PhasorDataModel::processFrame(const qpmu::Measurement_Frame &frame)
{
    if (m_isPaused) {
        return;
    }

    QVariantList magnitudes;
    QVariantList phases;
    QVariantList freqs;

    // Extract data from measurement frame
    for (size_t i = 0; i < qpmu::N_Channels; ++i) {
        const auto &estimate = frame.estimate_vector[i];

        qreal magnitude = std::abs(estimate.phasor);
        qreal phase = std::arg(estimate.phasor) * 180.0 / M_PI; // Convert to degrees
        qreal frequency = estimate.frequency;

        magnitudes.append(magnitude);
        phases.append(phase);
        freqs.append(frequency);
    }

    // Calculate sampling rate from timestamp if available
    qreal samplingRate = m_samplingRate; // Keep previous or calculate from delta
    qint64 timestamp = frame.timestamp;

    updateProperties(magnitudes, phases, freqs, samplingRate, timestamp);
}
