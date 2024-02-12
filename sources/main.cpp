#include <QApplication>

#include "App.h"
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    App app(argc, argv);

    QThread::sleep(1);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
