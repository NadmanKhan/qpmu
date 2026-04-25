#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>

#include "qpmu/core.h"

/// IPC client for receiving Measurement_Frame data over a Unix domain socket.
/// Reconnects automatically with exponential backoff (100ms → 5s).
class GuiIpcClient : public QObject
{
    Q_OBJECT

public:
    explicit GuiIpcClient(QObject *parent = nullptr);
    ~GuiIpcClient() override;

    Q_INVOKABLE bool connectToService(const QString &socketPath = QStringLiteral("/tmp/qpmu_gui.sock"));
    Q_INVOKABLE void disconnectFromService();

    bool isConnected() const { return m_connected; }
    QString lastError() const { return m_lastError; }

signals:
    void frameReceived(const qpmu::Measurement_Frame &frame);
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

private slots:
    void onSocketReadyRead();
    void attemptReconnect();

private:
    bool createConnection(const QString &socketPath);
    void closeConnection();
    void scheduleReconnect();

    int m_sockfd = -1;
    QSocketNotifier *m_socketNotifier = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    bool m_connected = false;
    QString m_socketPath;
    QString m_lastError;

    QByteArray m_readBuffer;
    static constexpr int FRAME_SIZE = sizeof(qpmu::Measurement_Frame);

    int m_reconnectDelay = 100;
    static constexpr int MAX_RECONNECT_DELAY = 5000;
};
