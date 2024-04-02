#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QPushButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>

#include "worker.h"
#include "phasor_view.h"
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

#endif // MAIN_WINDOW_H
