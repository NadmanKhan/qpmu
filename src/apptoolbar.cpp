#include "apptoolbar.h"

#include <QDebug>

AppToolBar::AppToolBar() {

    setMovable(false);

    auto palette = QPalette();
    palette.setColor(QPalette::Window, Qt::lightGray);
    setPalette(palette);

    const auto cw = AppCentralWidget::ptr();
    const auto dw = AppDockWidget::ptr();

    m_returnAct = new QAction(QIcon(":/images/return.png"), tr("Go back"));
    m_returnAct->setShortcut(QKeySequence::Back);
    m_returnAct->setEnabled(cw->currentIndex() > 0);
    connect(m_returnAct, &QAction::triggered, cw, &AppCentralWidget::pop);
    connect(cw, &AppCentralWidget::currentChanged, m_returnAct, [this](int i) {
        m_returnAct->setEnabled(i > 0);
    });
    addAction(m_returnAct);

    m_openControlAct = new QAction(QIcon(":/images/control-panel.png"), tr("Open control panel"));
    m_openControlAct->setCheckable(true);
    m_openControlAct->setChecked(false);
    m_openControlAct->setEnabled(false);
    connect(m_openControlAct, &QAction::toggled, dw, &QDockWidget::setVisible);
    addAction(m_openControlAct);
}

AppToolBar* AppToolBar::m_ptr = nullptr;

AppToolBar* AppToolBar::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppToolBar();
    return m_ptr;
}

void AppToolBar::setControlEnabled(bool enabled) {
    m_openControlAct->setEnabled(enabled);
}

void AppToolBar::setControlChecked(bool checked) {
    m_openControlAct->setChecked(checked);
}
