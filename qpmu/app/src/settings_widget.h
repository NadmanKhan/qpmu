#ifndef QPMU_APP_SETTINGS_WIDGET_H
#define QPMU_APP_SETTINGS_WIDGET_H

#include "qpmu/common.h"

#include <QWidget>
#include <QTabWidget>

class SettingsWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

private:
    QWidget *createSampleSourcePage();
    QWidget *createCalibrationPage();
    QWidget *createSignalCalibrationWidget(qpmu::USize signalIndex);

private:
};

#endif // QPMU_APP_SETTINGS_WIDGET_H