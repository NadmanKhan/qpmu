#ifndef QPMU_APP_SETTINGS_WIDGET_H
#define QPMU_APP_SETTINGS_WIDGET_H

#include "qpmu/defs.h"
#include "settings_models.h"
#include "main_page_interface.h"

#include <QWidget>
#include <QSettings>
#include <QString>
#include <QHostAddress>
#include <QTabWidget>
#include <QAbstractSocket>
#include <QIODevice>

#include <vector>

class SettingsWidget : public QTabWidget, public MainPageInterface
{
    Q_OBJECT
    Q_INTERFACES(MainPageInterface)

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    void visualisationSettingsChanged(const VisualisationSettings &settings);

private:
    QWidget *networkSettingsPage(const NetworkSettings &settings);
    QWidget *calibrationWidget(const size_t signalIndex,
                               const CalibrationSettings::DataPerSignal &signalData);

    NetworkSettings m_oldNetworkSettings;
    CalibrationSettings m_oldCalibrationSettings;
};

#endif // QPMU_APP_SETTINGS_WIDGET_H