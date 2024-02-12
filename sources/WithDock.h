//
// Created by nadman on 11/13/23.
//

#ifndef WITHDOCK_H
#define WITHDOCK_H

#include <QDockWidget>
#include <QMetaType>

class WithDock
{

public:
    WithDock() = default;
    WithDock(const WithDock &other) = default;
    explicit WithDock(bool hasDock);
    virtual ~WithDock();

    [[nodiscard]] QDockWidget *dockWidget() const;
};

Q_DECLARE_METATYPE(WithDock)
Q_DECLARE_INTERFACE(WithDock, "WithDock")

#endif // WITHDOCK_H
