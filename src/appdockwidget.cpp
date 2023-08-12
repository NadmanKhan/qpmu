#include "appdockwidget.h"

AppDockWidget::AppDockWidget() {
    setFeatures(QDockWidget::NoDockWidgetFeatures);

    auto palette = QPalette();
    palette.setColor(QPalette::Window, Qt::darkGray);
    setPalette(palette);

    hide();
}

AppDockWidget* AppDockWidget::m_ptr = nullptr;

AppDockWidget* AppDockWidget::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppDockWidget();
    return m_ptr;
}
