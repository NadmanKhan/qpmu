#ifndef APPDOCKWIDGET_H
#define APPDOCKWIDGET_H

#include <QDockWidget>

class AppDockWidget : public QDockWidget {
    Q_OBJECT

public:
    static AppDockWidget* ptr();

private:
    AppDockWidget();

    static AppDockWidget* m_ptr;
};

#endif // APPDOCKWIDGET_H
