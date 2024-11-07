#include "app.h"
#include "main_window.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    App app(argc, argv);

    app.mainWindow()->show();
    
    return app.exec();
}