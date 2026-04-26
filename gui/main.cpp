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

    // ── Dark palette ────────────────────────────────────────────────────────
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

    // ── Core objects ────────────────────────────────────────────────────────
    auto *model = new SignalDataModel(&app);
    auto *ipcClient = new GuiIpcClient(&app);
    auto *window = new MainWindow(model);
    window->show();

    auto updateStatusBar = [&]() {
        if (model->isPaused())
            return;
        auto now = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
        window->setLastSampleTime(now);
        qreal freq = model->data(model->index(0, 0), SignalDataModel::FrequencyRole).toReal();
        window->setSystemFrequency(freq);
    };

    // ── IPC throttle (2 Hz) ─────────────────────────────────────────────────
    qpmu::Measurement_Frame pendingFrame{};
    bool hasPending = false;

    QTimer ipcThrottle;
    ipcThrottle.setInterval(500);
    QObject::connect(&ipcThrottle, &QTimer::timeout, &app, [&]() {
        if (hasPending) {
            model->updateFromFrame(pendingFrame);
            hasPending = false;
            updateStatusBar();
        }
    });

    QObject::connect(ipcClient, &GuiIpcClient::frameReceived, &app,
                     [&](const qpmu::Measurement_Frame &frame) {
        pendingFrame = frame;
        hasPending = true;
        if (!ipcThrottle.isActive()) {
            model->updateFromFrame(pendingFrame);
            hasPending = false;
            ipcThrottle.start();
        }
    });

    // ── Connection lifecycle ────────────────────────────────────────────────
    QObject::connect(ipcClient, &GuiIpcClient::connected, &app, [&]() {
        window->setLiveMode(true);
    });

    QObject::connect(ipcClient, &GuiIpcClient::disconnected, &app, [&]() {
        ipcThrottle.stop();
        hasPending = false;
        window->setLiveMode(false);
    });

    QTimer::singleShot(100, &app, [&]() {
        ipcClient->connectToService();
    });

    return app.exec();
}
