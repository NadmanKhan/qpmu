#include "appmainwindow.h"

AppMainWindow::AppMainWindow() {
    const auto cw = AppCentralWidget::ptr();
    const auto mm = AppMainMenu::ptr();
    const auto tb = AppToolBar::ptr();
    const auto dw = AppDockWidget::ptr();
    const auto sb = AppStatusBar::ptr();

    addDockWidget(Qt::LeftDockWidgetArea, dw);
    addToolBar(Qt::LeftToolBarArea, tb);
    setStatusBar(sb);
    setCentralWidget(cw);
    cw->addWidget(mm);
    mm->addItem(
        tr("Wave View"),
        QIcon(":/images/wave-graph.png"),
        new PhasorWaveView(this));
    mm->addItem(
        tr("Polar View"),
        QIcon(":/images/polar-chart.png"),
        new PhasorPolarView(this));

}

AppMainWindow* AppMainWindow::m_ptr = nullptr;

AppMainWindow* AppMainWindow::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppMainWindow();
    return m_ptr;
}
