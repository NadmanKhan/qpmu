#include "App.h"

App::App(int &argc, char **argv)
    : QApplication(argc, argv)
{
    // Register meta types
    // -------------------

    (void) qRegisterMetaType<AdcSampleVector>("AdcSampleVector");

    // Set application information
    // ---------------------------

    QApplication::setOrganizationName("CPS Lab - NSU");
    QApplication::setApplicationName("QPMU");
    QApplication::setApplicationVersion("0.1.0");

    // Get config path: read environment variables or parse command line
    // -----------------------------------------------------------------

    QString configPath = QDir().dirName() + "/qpmu.ini"; // default config path

    // Get the config path from environment variable if it exists
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("QPMU_CONFIG_PATH")) {
        configPath = env.value("QPMU_CONFIG_PATH");
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("QPMU is a GUI application for "
                                     "PMU control and data visualization.");
    parser.addHelpOption(); // --help
    auto configFileOption = QCommandLineOption(QStringList() << "C"
                                                             << "config",
                                               "Path to config file.", "path",
                                               configPath); // -C, --config
    parser.addOption(configFileOption);
    parser.process(*this);

    // Get the config path from command line if it exists
    if (parser.isSet(configFileOption)) {
        configPath = parser.value(configFileOption);
    }

    qDebug() << "Config path: " << configPath;

    // Settings (must be initialize BEFORE data model)
    // -----------------------------------------------

    m_settings = new QSettings(configPath, QSettings::IniFormat);
    m_settings->sync();

    // Data model
    // ----------

    m_modelThread  = new QThread();
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
