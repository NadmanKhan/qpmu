#ifndef QPMU_APP_DATA_OBSERVER_H
#define QPMU_APP_DATA_OBSERVER_H

#include "app.h"
#include "qpmu/defs.h"
#include "settings_models.h"
#include "settings_widget.h"
#include "data_processor.h"

#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>
#include <QVector>

#include <array>

class DataObserver : public QObject
{
    Q_OBJECT

public:
    using USize = qpmu::USize;
    using SignalColors = std::array<QColor, qpmu::CountSignals>;

    explicit DataObserver();

signals:
    void sampleBufferUpdated(const SampleStoreBuffer &sampleBuffer);
    void estimationUpdated(const qpmu::Estimation &estimation);

private slots:
    void update();

private:
    USize m_updateCounter = 0;
};

#endif // QPMU_APP_DATA_OBSERVER_H