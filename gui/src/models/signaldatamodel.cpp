#include "signaldatamodel.h"
#include <QtMath>
#include <QSize>

// Define color palette
const QColor SignalDataModel::s_colorPalette[qpmu::N_Channels] = {
    QColor("#ff6b6b"), // VA - Coral red
    QColor("#4ecdc4"), // VB - Teal
    QColor("#45b7d1"), // VC - Sky blue
    QColor("#ffd93d"), // IA - Golden yellow
    QColor("#c56cf0"), // IB - Purple
    QColor("#95e1d3") // IC - Mint
};

const QList<SignalDataModel::TableColumnConfig> SignalDataModel::s_tableColumnConfigs = {
    {
            "Magnitude",
            120,
            [](const SignalData *s) {
                return QString::number(s->magnitude(), 'f', 2) + " " + s->unit();
            },
    },
    {
            "Phase",
            100,
            [](const SignalData *s) { return QString::number(s->phase(), 'f', 1) + "°"; },
    },
    {
            "Real Power",
            120,
            [](const SignalData *s) { return QString::number(s->realPower(), 'f', 1) + " W"; },
    },
    {
            "Reactive Power",
            120,
            [](const SignalData *s) {
                return QString::number(s->reactivePower(), 'f', 1) + " VAR";
            },
    }
};

SignalDataModel::SignalDataModel(QObject *parent) : QAbstractItemModel(parent)
{
    initializeSignals();
}

// HELPERS

void SignalDataModel::initializeSignals()
{
    // Create signal data from core Signal_Info array
    m_signals.reserve(qpmu::N_Channels);

    for (std::size_t i = 0; i < qpmu::N_Channels; ++i) {
        SignalData *signal = new SignalData(qpmu::Signals[i], s_colorPalette[i], this);
        m_signals.append(signal);
    }
}

bool SignalDataModel::isValidIndex(const QModelIndex &index) const
{
    return
            // the index must be valid
            index.isValid()
            // the parent index must be invalid (flat model)
            && !index.parent().isValid()
            // the row is within the bounds of the signal list
            && (index.row() >= 0 && index.row() < m_signals.count())
            // the column is within the defined column configs
            && (index.column() >= 0
                && index.column() < SignalDataModel::s_tableColumnConfigs.size());
}

// MODEL OVERRIDES

QModelIndex SignalDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex(); // Flat model, no children
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
    return SignalDataModel::s_tableColumnConfigs.size();
}

QVariant SignalDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (!isValidIndex(index(0, section)))
            return QVariant();

        const auto &config = SignalDataModel::s_tableColumnConfigs[section];

        switch (role) {
        case Qt::DisplayRole:
            return config.header;
        case Qt::SizeHintRole:
            return QSize(config.widthHint, 50); // Column width hint, height not used by headers
        }

    } else /* if (orientation == Qt::Vertical) */ {
        if (!isValidIndex(index(section, 0)))
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return m_signals[section]->name();
        case Qt::SizeHintRole:
            return QSize(60, 50); // Row height
        }
    }

    return QVariant();
}

QVariant SignalDataModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index))
        return QVariant();

    SignalData *signal = m_signals.at(index.row());
    const auto &config = SignalDataModel::s_tableColumnConfigs[index.column()];
    const auto &signalColor = signal->color();

    switch (role) {
    case Qt::DisplayRole:
        return config.formatValue(signal);
    case Qt::DecorationRole:
        return signalColor;
    case Qt::BackgroundRole:
        return QColor(signalColor.red(), signalColor.green(), signalColor.blue(),
                      255 * 0.15); // 15% opacity
    case Qt::ForegroundRole:
        return signalColor;
    case SignalDataRole:
        return QVariant::fromValue(signal);
    }

    return QVariant();
}

QHash<int, QByteArray> SignalDataModel::roleNames() const
{
    static QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display"; // Standard Qt role for display text
    roles[Qt::ForegroundRole] = "foreground"; // Text color
    roles[Qt::BackgroundRole] = "background"; // Background color
    roles[Qt::DecorationRole] = "decoration"; // Icon/decoration color
    roles[Qt::SizeHintRole] = "sizeHint"; // Size hint
    roles[SignalDataRole] = "signalData";
    return roles;
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

        qreal vMag = m_signals[vIndex]->magnitude();
        qreal vPhase = m_signals[vIndex]->phase();
        qreal iMag = m_signals[iIndex]->magnitude();
        qreal iPhase = m_signals[iIndex]->phase();

        // Phase difference between voltage and current (in degrees)
        qreal phaseDiff = vPhase - iPhase;
        qreal phaseDiffRad = phaseDiff * M_PI / 180.0;

        // Compute power: P = V * I * cos(θ), Q = V * I * sin(θ)
        qreal apparentPower = vMag * iMag;
        qreal realPower = apparentPower * qCos(phaseDiffRad);
        qreal reactivePower = apparentPower * qSin(phaseDiffRad);

        // Update both voltage and current signals with the same power values
        m_signals[vIndex]->updatePower(realPower, reactivePower);
        m_signals[iIndex]->updatePower(realPower, reactivePower);
    }
}

void SignalDataModel::updateFromFrame(const qpmu::Measurement_Frame &frame, bool isPaused)
{
    if (isPaused) {
        return;
    }

    // Update each signal from the measurement frame
    for (std::size_t i = 0; i < qpmu::N_Channels; ++i) {
        m_signals[i]->updateFromMeasurement(frame.sample_vector[i], frame.estimate_vector[i]);
    }

    // Compute power for each phase
    computePower();

    // Notify views that data has changed (all rows, all columns)
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void SignalDataModel::updateSimulatedData(qreal simulationTime, bool isPaused)
{
    if (isPaused) {
        return;
    }

    // Simulate 3-phase voltage and current phasors
    qreal vMag = 120.0 + 5.0 * qSin(simulationTime * 0.5);
    qreal iMag = 10.0 + 1.0 * qSin(simulationTime * 0.7);
    qreal freqVariation = 60.0 + 0.05 * qSin(simulationTime * 0.3);

    // Update each signal
    for (std::size_t i = 0; i < qpmu::N_Channels; ++i) {
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

        m_signals[i]->updateFromEstimate(estimate);
    }

    // Compute power for each phase
    computePower();

    // Notify views that data has changed (all rows, all columns)
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}
