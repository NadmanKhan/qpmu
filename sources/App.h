#ifndef APP_H
#define APP_H

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QMetaType>
#include <QSettings>
#include <QThread>

#include "AdcDataModel.h"

class AdcDataModel; // forward declaration

class App: public QApplication
{
Q_OBJECT

public:
    explicit App(int &argc, char **argv);

private:
    QSettings *m_settings = nullptr;
    QThread *m_modelThread = nullptr;
    AdcDataModel *m_adcDataModel = nullptr;

public:
    [[nodiscard]] AdcDataModel *adcDataModel() const;
    [[nodiscard]] QSettings *settings();
};


#endif //APP_H
