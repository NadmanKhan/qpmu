#include <QtWidgets/QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>

#include <QQmlEngine>
#include "worker.h"
#include "main_window.h"

#include <iostream>
#include <iomanip>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);

    std::cout << std::fixed << std::setprecision(2);

    //    QQmlApplicationEngine engine;

    //    const QUrl url(QStringLiteral("qrc:/main.qml"));
    //    QObject::connect(
    //            &engine, &QQmlApplicationEngine::objectCreated, &app,
    //            [url](QObject *obj, const QUrl &objUrl) {
    //                if (!obj && url == objUrl)
    //                    QCoreApplication::exit(-1);
    //            },
    //            Qt::QueuedConnection);

    //    QProcess adc;
    //    adc.setProgram("python3");
    //    adc.setArguments(QStringList() << "adc_simulator.py");

    //    Worker worker(&adc);

    //    worker.start();

    //    engine.rootContext()->setContextProperty("worker", &worker);
    //    engine.load(url);

    QProcess adc;
    adc.setProgram("python3");
    adc.setArguments(QStringList() << "adc_simulator.py");
    auto worker = new Worker(&adc);

    MainWindow window(worker);
    worker->start();
    window.resize(600, 250);
    window.show();

    return app.exec();
}
