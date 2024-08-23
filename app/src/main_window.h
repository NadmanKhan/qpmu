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

#include "app.h"
#include "phasor_view.h"
#include "waveform_view.h"
#include "monitor_view.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:

private:
    QStackedWidget *m_stack = nullptr;
};

#endif // MAIN_WINDOW_H
