#ifndef APPSTATUSBAR_H
#define APPSTATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QDateTime>
#include <QTimer>

class AppStatusBar : public QStatusBar {
    Q_OBJECT

public:
    static AppStatusBar* ptr();

private slots:
    void updateDateTime();

private:
    AppStatusBar();
    static QLabel* makeLabel();

    static AppStatusBar* m_ptr;
    QLabel* m_dateLabel;
    QLabel* m_timeLabel;
    QTimer* m_timer;
};

#endif // APPSTATUSBAR_H
