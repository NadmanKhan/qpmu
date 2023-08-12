#include "appcentralwidget.h"
#include <QDebug>

AppCentralWidget::AppCentralWidget() = default;

AppCentralWidget* AppCentralWidget::m_ptr = nullptr;

AppCentralWidget* AppCentralWidget::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppCentralWidget();
    return m_ptr;
}

void AppCentralWidget::push(QWidget* w) {
    setCurrentIndex(addWidget(w));
}

void AppCentralWidget::pop() {
    int i = currentIndex();
    setCurrentIndex(i - 1);
    removeWidget(widget(i));
}
