#include "app.h"
#include "data_processor.h"
#include "main_window.h"

#include <QFont>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QProcess>
#include <QFile>
#include <QAbstractSocket>

App::App(int &argc, char **argv) : QApplication(argc, argv)
{
    setOrganizationName(qpmu::OrgName);
    setApplicationName(qpmu::AppName);
    setApplicationDisplayName(qpmu::AppDisplayName);

    {
        auto font = this->font();
        font.setPointSize(1.25 * font.pointSize());
        setFont(font);
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(ViewUpdateIntervalMs);
    m_timer->start();

    m_dataProcessor = new DataProcessor();
    m_dataProcessor->start();
    m_dataProcessor->setPriority(QThread::TimeCriticalPriority);

    m_mainWindow = new MainWindow();
}
