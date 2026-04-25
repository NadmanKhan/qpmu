#pragma once

#include <QWidget>

class ContextItemModel;

class Screen : public QWidget
{
    Q_OBJECT

public:
    explicit Screen(QWidget *parent = nullptr) : QWidget(parent) {}

    virtual QString title() const = 0;
    virtual ContextItemModel *contextModel() const { return nullptr; }

signals:
    void navigateTo(Screen *screen);
};
