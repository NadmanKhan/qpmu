#include <QFont>

#include "app.h"

App::App(int &argc, char **argv) : QApplication(argc, argv)
{
    setOrganizationName("CPS Lab - NSU");
    setApplicationName("QPMU");

    {
        auto font = this->font();
        font.setPointSize(font.pointSize() * 1.25);
        setFont(font);
    }

    // Before instantiating any `QSettings` object using the default ctor as below, make
    // sure the org and app names are set using `QCoreApplication::setOrganizationName`
    // and `QCoreApplication::setOrganizationName`. The file path is created using them.
    m_settings = new QSettings(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    m_timer->start();

    m_worker = new Worker();
    m_worker->start();
}

QSettings *App::settings()
{
    return m_settings;
}

QTimer *App::timer()
{
    return m_timer;
}

Worker *App::worker()
{
    return m_worker;
}