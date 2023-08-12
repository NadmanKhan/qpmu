#include "appmainmenu.h"

AppMainMenu* AppMainMenu::m_ptr = nullptr;

AppMainMenu* AppMainMenu::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppMainMenu(3);
    return m_ptr;
}

AppMainMenu::AppMainMenu(quint8 maxColumns) : maxColumns(maxColumns) {
    m_gridLayout = new QGridLayout(this);
}

void AppMainMenu::addItem(QString name, QIcon icon, QWidget* target) {
    auto item = new QWidget(this);

    auto pushButton = new QPushButton(icon, "", item);
    pushButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pushButton->setMinimumSize(120, 120);
    pushButton->setIconSize(0.8 * pushButton->size());

    auto label = new QLabel(name, item);
    label->setAlignment(Qt::AlignTop | Qt::AlignCenter);

    auto btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    btnLayout->addWidget(pushButton, 1);
    btnLayout->addStretch(1);

    auto itemLayout = new QVBoxLayout(item);
    itemLayout->addStretch(1);
    itemLayout->addLayout(btnLayout, 1);
    itemLayout->addWidget(label, 0);
    itemLayout->addStretch(1);

    item->setLayout(itemLayout);

    int row = m_gridLayout->count() / maxColumns;
    int column = m_gridLayout->count() % maxColumns;
    m_gridLayout->addWidget(item, row, column);

    const auto cw = AppCentralWidget::ptr();
    connect(pushButton, &QPushButton::clicked, cw, [cw, target] {
        cw->push(target);
    });
}
