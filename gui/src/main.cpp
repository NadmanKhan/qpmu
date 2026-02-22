#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Set application metadata
    QGuiApplication::setApplicationName("QPMU");
    QGuiApplication::setApplicationDisplayName("Phasor Measurement Unit");
    QGuiApplication::setOrganizationName("cps-lab-nsu");

    QQmlApplicationEngine engine;
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("qpmu", "Main");

    return app.exec();
}
