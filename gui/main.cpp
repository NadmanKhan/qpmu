#include <QApplication>
#include <QPalette>
#include <QTimer>
#include <QDateTime>

#include "mainwindow.h"
#include "signaldatamodel.h"
#include "guiclient.h"
#include "theme.h"
#include "qpmu/core.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QPMU"));
    app.setApplicationDisplayName(QStringLiteral("Phasor Measurement Unit"));

    // Dark palette
    QPalette pal;
    pal.setColor(QPalette::Window, Theme::Colors::background);
    pal.setColor(QPalette::WindowText, Theme::Colors::textPrimary);
    pal.setColor(QPalette::Base, Theme::Colors::surface);
    pal.setColor(QPalette::AlternateBase, Theme::Colors::surfaceElevated);
    pal.setColor(QPalette::Text, Theme::Colors::textPrimary);
    pal.setColor(QPalette::Button, Theme::Colors::surfaceElevated);
    pal.setColor(QPalette::ButtonText, Theme::Colors::textPrimary);
    pal.setColor(QPalette::Highlight, Theme::Colors::primary);
    pal.setColor(QPalette::HighlightedText, Theme::Colors::background);
    pal.setColor(QPalette::Mid, Theme::Colors::border);
    pal.setColor(QPalette::Dark, Theme::Colors::borderSubtle);
    app.setPalette(pal);

    app.setStyleSheet(QStringLiteral("* { font-family: '%1'; }").arg(Theme::Font::family));

    // Data model + IPC client
    auto *model = new SignalDataModel(&app);
    auto *ipcClient = new GuiIpcClient(&app);

    // Window
    auto *window = new MainWindow(model);
    window->show();

    // --- 2Hz IPC throttle ---
    qpmu::Measurement_Frame pendingFrame{};
    bool hasPending = false;

    QTimer ipcThrottle;
    ipcThrottle.setInterval(500);
    QObject::connect(&ipcThrottle, &QTimer::timeout, &app, [&]() {
        if (hasPending) {
            model->updateFromFrame(pendingFrame);
            hasPending = false;

            if (!model->isPaused()) {
                auto ms = pendingFrame.timestamp / (qpmu::Time_Resolution / 1000);
                window->setLastSampleTime(
                    QDateTime::fromMSecsSinceEpoch(ms).toString(QStringLiteral("hh:mm:ss.zzz")));
                qreal freq = model->data(model->index(0, 0),
                                         SignalDataModel::FrequencyRole).toReal();
                window->setSystemFrequency(freq);
            }
        }
    });

    QObject::connect(ipcClient, &GuiIpcClient::frameReceived, &app,
                     [&](const qpmu::Measurement_Frame &frame) {
        pendingFrame = frame;
        hasPending = true;
        if (!ipcThrottle.isActive()) {
            // First frame: process immediately, then throttle
            model->updateFromFrame(pendingFrame);
            hasPending = false;
            ipcThrottle.start();
        }
    });

    // --- 2Hz simulation fallback ---
    qreal simTime = 0.0;
    QTimer simTimer;
    simTimer.setInterval(500);
    QObject::connect(&simTimer, &QTimer::timeout, &app, [&]() {
        simTime += 0.5;
        model->updateSimulatedData(simTime);
        if (!model->isPaused()) {
            window->setLastSampleTime(
                QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz")));
            qreal freq = model->data(model->index(0, 0),
                                     SignalDataModel::FrequencyRole).toReal();
            window->setSystemFrequency(freq);
        }
    });

    // --- Connection lifecycle ---
    QObject::connect(ipcClient, &GuiIpcClient::connected, &app, [&]() {
        simTimer.stop();
        window->setLiveMode(true);
    });

    QObject::connect(ipcClient, &GuiIpcClient::disconnected, &app, [&]() {
        ipcThrottle.stop();
        hasPending = false;
        window->setLiveMode(false);
        if (!simTimer.isActive())
            simTimer.start();
    });

    // Start: try IPC, fall back to simulation
    QTimer::singleShot(100, &app, [&]() {
        ipcClient->connectToService();
    });
    simTimer.start();

    return app.exec();
}
