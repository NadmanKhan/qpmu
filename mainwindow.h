#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QPushButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>

#include "worker.h"
#include "phasorview.h"
#include <QProcess>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Worker *worker, QWidget *parent = nullptr);

signals:

private:
    QStackedWidget *m_stack = nullptr;
};

#endif // MAINWINDOW_H
