#include "appmainwindow.h"
#include "phasor.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication app(argc, argv);

    Phasor::setup();

    auto mainWindow = AppMainWindow::ptr();
    mainWindow->show();

    return app.exec();
}
