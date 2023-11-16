#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QDateTime>
#include <QToolBar>

#include "WaveformView.h"

class MainWindow: public QMainWindow
{
Q_OBJECT

public:
    explicit MainWindow();

private:
    // Widgets
    QStackedWidget *stackedWidget = nullptr;
    QToolBar *toolBar = nullptr;
    QLabel *statusBarTimeLabel = nullptr;
    QLabel *statusBarDateLabel = nullptr;
    WaveformView *waveformView = nullptr;

    // Toolbar actions
    QAction *goBackAction = nullptr;

    // Other objects
    QTimer *timer = nullptr;

private slots:
    void goBack();
    void navigateTo(QWidget *widget);
    void handleWidgetRemoved(int index);
    void handleWidgetChanged(int index);
    void handleTimeout();
};


#endif // MAINWINDOW_H
