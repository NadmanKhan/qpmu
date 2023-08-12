#ifndef APPMAINMENU_H
#define APPMAINMENU_H

#include "appcentralwidget.h"
#include "apptoolbar.h"

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

class AppMainMenu : public QWidget {
    Q_OBJECT
public:
    static AppMainMenu* ptr();
    void addItem(QString name, QIcon icon, QWidget* target = nullptr);

    const quint8 maxColumns;

private:
    AppMainMenu(quint8 maxColumns);

    static AppMainMenu* m_ptr;
    QGridLayout* m_gridLayout;
};

#endif // APPMAINMENU_H
