#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "application.h"

int main(int argc, char *argv[])
{
    // On-demand rendering only — prevents vsync-locked scenegraph compositing
    qputenv("QSG_RENDER_LOOP", "basic");

    Application app(argc, argv);

    // Set application metadata
    Application::setApplicationName("QPMU");
    Application::setApplicationDisplayName("Phasor Measurement Unit");
    Application::setOrganizationName("cps-lab-nsu");

    QQmlApplicationEngine engine;

    // Expose application instance and models to QML
    engine.rootContext()->setContextProperty("appInstance", &app);
    engine.rootContext()->setContextProperty("signalDataModel", app.signalDataModel());

    // Load QML from qt_add_qml_module generated resources (QTP0001 NEW prefix)
    const QUrl url(QStringLiteral("qrc:/qt/qml/qpmu/src/views/Main.qml"));

    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
