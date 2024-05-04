#ifndef TIMEOUTNOTIFIER_H
#define TIMEOUTNOTIFIER_H

#include <QObject>
#include <QtGlobal>
#include <QTimer>

class TimeoutNotifier : public QObject
{
    Q_OBJECT
public:
    explicit TimeoutNotifier(QObject *parent = nullptr);
    TimeoutNotifier(QTimer *timer, QObject *parent = nullptr);
    TimeoutNotifier(QTimer *timer, quint32 targetIntervalMs, QObject *parent = nullptr);

    bool isTimeout() const;

public slots:
    void start();
    void stop();

private slots:
    void update();

signals:
    void timeout();

private:
    QTimer *m_timer;
    quint32 m_target = 1;
    quint32 m_counter = 0;
    bool m_running = false;
};

#endif // TIMEOUTNOTIFIER_H
