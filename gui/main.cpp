#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "src/phasordatamodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application metadata
    QGuiApplication::setApplicationName("QPMU");
    QGuiApplication::setApplicationDisplayName("Phasor Measurement Unit");
    QGuiApplication::setOrganizationName("cps-lab-nsu");

    // Register QML types
    qmlRegisterType<PhasorDataModel>("QPMU", 1, 0, "PhasorDataModel");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qpmu", "Main");

    return app.exec();
}
