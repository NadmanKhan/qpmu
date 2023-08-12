#include "appstatusbar.h"

AppStatusBar::AppStatusBar() {

    m_dateLabel = makeLabel();
    m_timeLabel = makeLabel();

    auto vline = new QFrame();
    vline->setFrameStyle(QFrame::VLine | QFrame::Sunken);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AppStatusBar::updateDateTime);
    m_timer->start(100);

    addPermanentWidget(m_timeLabel);
    addPermanentWidget(vline);
    addPermanentWidget(m_dateLabel);
}

QLabel *AppStatusBar::makeLabel() {
    auto label = new QLabel();

    auto font = label->font();
    font.setFamilies({"Courier", "Roboto Mono", "Source Code Pro", "Consolas"});
    label->setFont(font);

    auto palette = label->palette();
    label->setPalette(palette);

    return label;
}

AppStatusBar* AppStatusBar::m_ptr = nullptr;

AppStatusBar* AppStatusBar::ptr() {
    if (m_ptr == nullptr) m_ptr = new AppStatusBar();
    return m_ptr;
}

void AppStatusBar::updateDateTime() {
    auto dateTime = QDateTime::currentDateTime();
    m_dateLabel->setText(dateTime.date().toString());
    m_timeLabel->setText(dateTime.time().toString());
}
