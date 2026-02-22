#ifndef SIGNAL_LIST_MODEL_H
#define SIGNAL_LIST_MODEL_H

#include <QAbstractItemModel>
#include <QColor>
#include <QObject>
#include <QtQmlIntegration>
#include <functional>

#include "qpmu/core.h"

// Individual signal data item - exposed to QML
class SignalData : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // Measurement data - raw sample
    Q_PROPERTY(quint16 sample READ sample NOTIFY dataChanged)

    // Measurement data - estimate
    Q_PROPERTY(qreal magnitude READ magnitude NOTIFY dataChanged)
    Q_PROPERTY(qreal phase READ phase NOTIFY dataChanged)
    Q_PROPERTY(qreal frequency READ frequency NOTIFY dataChanged)
    Q_PROPERTY(qreal rocof READ rocof NOTIFY dataChanged)

    // Computed power values (only meaningful for voltage/current pairs)
    Q_PROPERTY(qreal realPower READ realPower NOTIFY dataChanged)
    Q_PROPERTY(qreal reactivePower READ reactivePower NOTIFY dataChanged)

    // Signal metadata (constant) - from qpmu::Signal_Info
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString unit READ unit CONSTANT)
    Q_PROPERTY(QString signalType READ signalType CONSTANT)
    Q_PROPERTY(QString phaseType READ phaseType CONSTANT)

    // Config data
    Q_PROPERTY(QColor color READ color CONSTANT)

    // For normalization in views
    Q_PROPERTY(qreal normalizedMagnitude READ normalizedMagnitude NOTIFY dataChanged)

public:
    explicit SignalData(const qpmu::Signal_Info &info, const QColor &color,
                        QObject *parent = nullptr)
        : QObject(parent),
          m_info(info),
          m_color(color),
          m_sample(0),
          m_magnitude(0.0),
          m_phase(0.0),
          m_frequency(60.0),
          m_rocof(0.0),
          m_realPower(0.0),
          m_reactivePower(0.0)
    {
    }

    // Getters - measurement data
    inline quint16 sample() const { return m_sample; }
    inline qreal magnitude() const { return m_magnitude; }
    inline qreal phase() const { return m_phase; }
    inline qreal frequency() const { return m_frequency; }
    inline qreal rocof() const { return m_rocof; }
    inline qreal realPower() const { return m_realPower; }
    inline qreal reactivePower() const { return m_reactivePower; }
    inline QString name() const { return QString::fromUtf8(m_info.name); }
    inline QString unit() const { return QString(QChar(m_info.unit_symbol)); }
    inline QString signalType() const
    {
        return m_info.type == qpmu::Signal_Info::Voltage ? QStringLiteral("Voltage")
                                                         : QStringLiteral("Current");
    }
    inline QString phaseType() const
    {
        return QStringLiteral("Phase_") + QString(QChar(m_info.phase_symbol));
    }
    inline QColor color() const { return m_color; }

    inline qreal normalizedMagnitude() const
    {
        // Normalize: Voltages to 0-1 range (assuming max ~300V), Currents (assuming max ~20A)
        return m_info.type == qpmu::Signal_Info::Voltage ? m_magnitude / 300.0 : m_magnitude / 20.0;
    }

    // Update methods
    inline void updateFromSample(const qpmu::Sample &sample)
    {
        m_sample = sample;
        emit dataChanged();
    }

    inline void updateFromEstimate(const qpmu::Estimate &estimate)
    {
        m_magnitude = std::abs(estimate.phasor);
        m_phase = std::arg(estimate.phasor) * 180.0 / M_PI; // Convert to degrees
        m_frequency = estimate.frequency;
        m_rocof = estimate.rocof;
        emit dataChanged();
    }

    inline void updateFromMeasurement(const qpmu::Sample &sample, const qpmu::Estimate &estimate)
    {
        m_sample = sample;
        m_magnitude = std::abs(estimate.phasor);
        m_phase = std::arg(estimate.phasor) * 180.0 / M_PI; // Convert to degrees
        m_frequency = estimate.frequency;
        m_rocof = estimate.rocof;
        emit dataChanged();
    }

    inline void updatePower(qreal realPower, qreal reactivePower)
    {
        m_realPower = realPower;
        m_reactivePower = reactivePower;
        // Note: dataChanged is emitted by updateFromMeasurement
    }

signals:
    void dataChanged();

private:
    // Signal metadata (constant)
    const qpmu::Signal_Info m_info;

    // Config data
    const QColor m_color;

    // Measurement data - raw sample
    qpmu::Sample m_sample;

    // Measurement data - estimate
    qreal m_magnitude;
    qreal m_phase;
    qreal m_frequency;
    qreal m_rocof;

    // Computed power values
    qreal m_realPower;
    qreal m_reactivePower;
};

// Item model managing all signal data
// Exposes collection of Signal objects as QAbstractItemModel for QML
// Can be used with TableView, ListView, and custom graph views
class SignalDataModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum SignalDataModelRoles {
        SignalDataRole = Qt::UserRole + 1,
    };
    Q_ENUM(SignalDataModelRoles)

    struct TableColumnConfig
    {
        QString header;
        int widthHint;
        std::function<QString(const SignalData *)> formatValue;
    };

    explicit SignalDataModel(QObject *parent = nullptr);

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

    // Update methods
    void updateFromFrame(const qpmu::Measurement_Frame &frame, bool isPaused);
    void updateSimulatedData(qreal simulationTime, bool isPaused);

private:
    void initializeSignals();
    void computePower();

    bool isValidIndex(const QModelIndex &index) const;

    // Signal data array
    QList<SignalData *> m_signals;

    // Color palette for signals
    static const QColor s_colorPalette[qpmu::N_Channels];

    static const QList<TableColumnConfig> s_tableColumnConfigs;
};

#endif // SIGNAL_LIST_MODEL_H
