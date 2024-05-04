#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QPushButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QProcess>
#include <QLabel>
#include <QTimer>

#include "worker.h"
#include "phasor_view.h"
#include "waveform_view.h"
#include "monitor_view.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(Worker *worker, QWidget *parent = nullptr);

signals:

private:
    QStackedWidget *m_stack = nullptr;
    QTimer *m_timer;
};

#endif // MAIN_WINDOW_H
