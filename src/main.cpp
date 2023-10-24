#include "appmainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <iostream>

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << "\n";

    QApplication app(argc, argv);

    auto mainWindow = AppMainWindow::ptr();
    mainWindow->show();

    return app.exec();
}
