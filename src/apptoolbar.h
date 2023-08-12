#ifndef APPTOOLBAR_H
#define APPTOOLBAR_H

#include "appcentralwidget.h"
#include "appdockwidget.h"
#include "appmainmenu.h"

#include <QAction>
#include <QToolBar>

class AppToolBar : public QToolBar {
    Q_OBJECT

public:
    static AppToolBar* ptr();

public slots:
    void setControlWidget(QWidget* controlWidget);

private:
    AppToolBar();

    static AppToolBar* m_ptr;
    QAction* m_returnAct;
    QAction* m_openControlAct;
};

#endif // APPTOOLBAR_H
