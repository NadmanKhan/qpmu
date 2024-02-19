#ifndef VISIBILITYCONTROLVIEW_H
#define VISIBILITYCONTROLVIEW_H

#include <QColor>
#include <QString>
#include <QWidget>

class VisibilityControlView: public QWidget
{
Q_OBJECT

public:
    struct TargetCategory
    {
        QString name;
    };

    struct Target: public QObject
    {
    Q_OBJECT
        QString name;
        QColor color;
        TargetCategory category;

    };

    explicit VisibilityControlView(QWidget *parent = nullptr);

private:

};


#endif //VISIBILITYCONTROLVIEW_H
