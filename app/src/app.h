#ifndef QPMU_APP_H
#define QPMU_APP_H

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QTimer>

#include "worker.h"

class App : public QApplication
{
#define APP (static_cast<App *>(QCoreApplication::instance()))

    Q_OBJECT

public:
    App(int &argc, char **argv);

    QSettings *settings();
    QTimer *timer();
    Worker *worker();

private:
    QSettings *m_settings = nullptr;
    QTimer *m_timer = nullptr;
    Worker *m_worker = nullptr;
};

#endif // QPMU_APP_H