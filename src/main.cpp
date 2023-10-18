#include "appmainwindow.h"
#include "phasor.h"

#include <QApplication>
#include <QDebug>

#include <iostream>
using std::cout;

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication app(argc, argv);

    qDebug() << "args:\n";
    Phasor::setup(Phasor::Live);

    auto mainWindow = AppMainWindow::ptr();
    mainWindow->show();

    return app.exec();
}
