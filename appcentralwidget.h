#ifndef APPCENTRALWIDGET_H
#define APPCENTRALWIDGET_H

#include <QStackedWidget>
#include <QWidget>

class AppCentralWidget : public QStackedWidget {
    Q_OBJECT

public:
    static AppCentralWidget* ptr();

public slots:
    void push(QWidget* w);
    void pop();

private:
    AppCentralWidget();

    static AppCentralWidget* m_ptr;
};

#endif // APPCENTRALWIDGET_H
