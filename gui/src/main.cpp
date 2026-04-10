#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "application.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);

    // Set application metadata
    Application::setApplicationName("QPMU");
    Application::setApplicationDisplayName("Phasor Measurement Unit");
    Application::setOrganizationName("cps-lab-nsu");

    QQmlApplicationEngine engine;

    // Add import path so engine can find the qpmu QML module
    engine.addImportPath("qrc:/");

    // Expose application instance and models to QML
    engine.rootContext()->setContextProperty("appInstance", &app);
    engine.rootContext()->setContextProperty("signalDataModel", app.signalDataModel());

    // Qt 6: Load QML from qt_add_qml_module generated resources
    const QUrl url(QStringLiteral("qrc:/qpmu/src/views/Main.qml"));

    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
