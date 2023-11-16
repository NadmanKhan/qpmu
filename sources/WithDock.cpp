#include "WithDock.h"

WithDock::WithDock(bool hasDock)
{
    if (hasDock) {
        dock = new QDockWidget();
        dock->setAllowedAreas(Qt::LeftDockWidgetArea);
        dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    } else {
        dock = nullptr;
    }
}

QDockWidget *WithDock::dockWidget() const
{
    return dock;
}

WithDock::~WithDock()
{
    delete dock;
}
