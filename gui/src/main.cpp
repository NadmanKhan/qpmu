#include <QtGlobal>

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#include "application.h"
#include "models/signaldatamodel.h"

int main(int argc, char *argv[])
{
    Application app(argc, argv);

    // Set application metadata
    Application::setApplicationName("QPMU");
    Application::setApplicationDisplayName("Phasor Measurement Unit");
    Application::setOrganizationName("cps-lab-nsu");

    QQmlApplicationEngine engine;

    // Manual type registration for both Qt 5 and Qt 6
    qmlRegisterType<SignalDataModel>("qpmu", 1, 0, "SignalDataModel");

    // Add import path for qpmu module (qmldir is at qrc:/qpmu/qmldir)
    engine.addImportPath("qrc:/");

    // Expose application instance and models to QML
    engine.rootContext()->setContextProperty("appInstance", &app);
    engine.rootContext()->setContextProperty("signalDataModel", app.signalDataModel());

    // Both Qt 5 and Qt 6: Use URL loading for consistency
    const QUrl url(QStringLiteral("qrc:/src/views/Main.qml"));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
#else
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreated, &app,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);
#endif

    engine.load(url);

    return app.exec();
}
