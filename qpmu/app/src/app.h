#ifndef QPMU_APP_H
#define QPMU_APP_H

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSettings>
#include <QTimer>
#include <QIODevice>
#include <QStringList>

#include "qpmu/common.h"

class Router;

class App : public QApplication
{
#define APP (static_cast<App *>(QCoreApplication::instance()))

    Q_OBJECT

public:
    App(int &argc, char **argv);

    QSettings *settings() const;
    QTimer *timer() const;
    Router *router() const;

private:
    QSettings *m_settings = nullptr;
    QTimer *m_timer = nullptr;
    Router *m_router = nullptr;
};

#endif // QPMU_APP_H