#include "mainwindow.h"

MainWindow::MainWindow(Worker *worker, QWidget *parent) : QMainWindow{ parent }
{
    m_stack = new QStackedWidget(this);

    auto firstView = new QWidget();
    auto gridlayout = new QGridLayout(firstView);
    firstView->setLayout(gridlayout);

    auto btnWaveform = new QPushButton();
    btnWaveform->setIcon(QIcon(":/images/wave-graph.png"));
    gridlayout->addWidget(btnWaveform, 0, 0);
    auto phasorView = new PhasorView(worker);

    auto btnPhasor = new QPushButton();
    btnPhasor->setIcon(QIcon(":/images/polar-chart.png"));
    gridlayout->addWidget(btnPhasor, 0, 1);
    //    connect(btnPhasor, &QPushButton::clicked,
    //            [&] { m_stack->setCurrentIndex(m_stack->addWidget(phasorView)); });

    m_stack->addWidget(firstView);
    setCentralWidget(m_stack);

    setCentralWidget(phasorView);
}
