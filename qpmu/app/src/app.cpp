#include <QFont>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QAbstractSocket>

#include "app.h"
#include "router.h"

App::App(int &argc, char **argv) : QApplication(argc, argv)
{
    setOrganizationName(qpmu::OrgName);
    setApplicationName(qpmu::AppName);
    setApplicationDisplayName(qpmu::AppDisplayName);

    {
        auto font = this->font();
        font.setPointSize(font.pointSize() * 1.25);
        setFont(font);
    }

    // Before instantiating any `Settings` object using the default ctor as below, make
    // sure the org and app names are set using `QCoreApplication::setOrganizationName`
    // and `QCoreApplication::setOrganizationName`. The file path is created using them.
    m_settings = new Settings(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    m_timer->start();

    m_router = new Router();
    m_router->start();
}

Settings *App::settings() const
{
    return m_settings;
}

QTimer *App::timer() const
{
    return m_timer;
}

Router *App::router() const
{
    return m_router;
}

QMainWindow *App::mainWindow() const
{
    return m_mainWindow;
}

void App::setMainWindow(QMainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
}
