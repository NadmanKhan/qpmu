#include <QApplication>

#include "App.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    App app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return QApplication::exec();
}
