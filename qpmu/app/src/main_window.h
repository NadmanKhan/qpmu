#ifndef QPMU_APP_MAIN_WINDOW_H
#define QPMU_APP_MAIN_WINDOW_H

#include "app.h"
#include "monitor_view.h"

#include <QMainWindow>
#include <QIcon>
#include <QPushButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QProcess>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:

private:
    QStackedWidget *m_stack = nullptr;
};

#endif // QPMU_APP_MAIN_WINDOW_H
