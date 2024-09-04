#ifndef QPMU_APP_H
#define QPMU_APP_H

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QTimer>
#include <QIODevice>
#include <QStringList>
#include <QMainWindow>

#include "qpmu/defs.h"
#include "settings.h"

class DataProcessor;

class App : public QApplication
{
#define APP (static_cast<App *>(QCoreApplication::instance()))

    Q_OBJECT

public:
    App(int &argc, char **argv);

    Settings *settings() const;
    QTimer *timer() const;
    DataProcessor *dataProcessor() const;
    QMainWindow *mainWindow() const;

    void setMainWindow(QMainWindow *mainWindow);

private:
    Settings *m_settings = nullptr;
    QTimer *m_timer = nullptr;
    QMainWindow *m_mainWindow = nullptr;
    DataProcessor *m_dataProcessor = nullptr;
};

#endif // QPMU_APP_H