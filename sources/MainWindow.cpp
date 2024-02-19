#include "MainWindow.h"
#include <QLineEdit>

MainWindow::MainWindow()
{
    setWindowTitle("QPMU");

    // Create a stacked widget to hold the pages of the application
    {
        setCentralWidget(stackedWidget = new QStackedWidget(this));
        connect(stackedWidget, &QStackedWidget::widgetRemoved, this, &MainWindow::handleWidgetRemoved);
        connect(stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::handleWidgetChanged);
    }

    // Create a toolbar to hold the buttons for the application
    {
        toolBar = new QToolBar(this);
        toolBar->setMovable(false);
        addToolBar(Qt::LeftToolBarArea, toolBar);
    }

    // Create the action for the back button
    {
        goBackAction = new QAction(this);
        goBackAction->setIcon(QIcon(":/images/return.png"));
        connect(goBackAction, &QAction::triggered, this, &MainWindow::goBack);
        toolBar->addAction(goBackAction);
        goBackAction->setEnabled(false);
    }

    // Create the status bar for the application
    {
        statusBar()->showMessage("Ready", 2000);
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
        timer->start(100);
        statusBarDateLabel = new QLabel(this);
        statusBarTimeLabel = new QLabel(this);
        auto separatorLabel = new QFrame(this);
        separatorLabel->setFrameStyle(QFrame::VLine | QFrame::Sunken);
        statusBar()->addPermanentWidget(statusBarTimeLabel);
        statusBar()->addPermanentWidget(separatorLabel);
        statusBar()->addPermanentWidget(statusBarDateLabel);
    }

    waveformView = new WaveformView(this);
    navigateTo(waveformView);

//    auto testInputWidget = new QWidget(this);
//    auto testInputLayout = new QVBoxLayout(testInputWidget);
//    auto testInputLabel = new QLabel("Test Input", testInputWidget);
//    testInputLayout->addWidget(testInputLabel);
//    auto testInputField = new QLineEdit(testInputWidget);
//    testInputLayout->addWidget(testInputField);
//    testInputLayout->addStretch(1);
//    navigateTo(testInputWidget);
}

void MainWindow::goBack()
{
    // Go back to the previous widget
    auto index = stackedWidget->currentIndex();
    Q_ASSERT(index > 0);
    stackedWidget->setCurrentIndex(index - 1);
    stackedWidget->removeWidget(stackedWidget->widget(index));
}

void MainWindow::navigateTo(QWidget *widget)
{
    // Add the new widget to the main window
    stackedWidget->addWidget(widget);
    stackedWidget->setCurrentWidget(widget);
    widget->show();
}

void MainWindow::handleWidgetChanged(int index)
{
    if (index == 0) {
        goBackAction->setEnabled(false);
    }
    else {
        goBackAction->setEnabled(true);
    }
}

void MainWindow::handleWidgetRemoved(int index)
{
}

void MainWindow::handleTimeout()
{
    auto dateTime = QDateTime::currentDateTime();
    auto date = dateTime.date();
    auto time = dateTime.time();
    statusBarTimeLabel->setText(time.toString(QStringLiteral("hh:mm:ss")));
    statusBarDateLabel->setText(date.toString(QStringLiteral("ddd dd MMM yyyy")));
}
