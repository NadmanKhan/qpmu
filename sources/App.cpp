#include "App.h"

App::App(int &argc, char **argv)
    : QApplication(argc, argv)
{
    // register meta types
    // --------------------------------------------------

    (void) qRegisterMetaType<AdcSampleVector>("AdcSampleVector");

    // set application information
    // --------------------------------------------------

    QApplication::setOrganizationName("CPS Lab - NSU");
    QApplication::setApplicationName("QPMU");
    QApplication::setApplicationVersion("0.1.0");

    // parse command line
    // --------------------------------------------------

    QString configPath = QDir().dirName() + "/qpmu.ini";

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("QPMU_CONFIG")) {
        configPath = env.value("QPMU_CONFIG");
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("QPMU is a GUI application for "
                                     "PMU control and data visualization.");
    parser.addHelpOption();
    parser.addVersionOption();
    auto configFileOption = QCommandLineOption(QStringList() << "C" << "config",
                                               "Path to config file.",
                                               "path",
                                               configPath);
    parser.addOption(configFileOption);
    parser.process(*this);

    configPath = parser.value(configFileOption);
    qDebug() << "config path: " << configPath;

    // settings
    // --------------------------------------------------

    m_settings = new QSettings(configPath, QSettings::IniFormat);
    m_settings->sync();
    qDebug() << "settings: " << m_settings->allKeys();

    // model
    // --------------------------------------------------

    m_modelThread = new QThread();
    m_adcDataModel = new AdcDataModel();
    m_adcDataModel->moveToThread(m_modelThread);
    Q_ASSERT(connect(m_modelThread, &QThread::finished,
                     m_adcDataModel, &AdcDataModel::deleteLater));
    m_modelThread->start();
}

AdcDataModel *App::adcDataModel() const
{
    return m_adcDataModel;
}

QSettings *App::settings()
{
    return m_settings;
}
