#include "guiclient.h"
#include <QDebug>
#include <cstring>

// Unix domain socket headers
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

GuiIpcClient::GuiIpcClient(QObject* parent)
    : QObject(parent)
{
    // Setup reconnection timer
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &GuiIpcClient::attemptReconnect);
}

GuiIpcClient::~GuiIpcClient()
{
    disconnectFromService();
}

bool GuiIpcClient::connectToService(const QString& socketPath)
{
    if (m_connected) {
        return true; // Already connected
    }

    m_socketPath = socketPath;

    if (createConnection(socketPath)) {
        m_reconnectDelay = 100; // Reset backoff on successful connection
        return true;
    }

    // Connection failed, schedule reconnect
    scheduleReconnect();
    return false;
}

void GuiIpcClient::disconnectFromService()
{
    m_reconnectTimer->stop();
    closeConnection();
}

bool GuiIpcClient::createConnection(const QString& socketPath)
{
    // Create Unix domain socket
    m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sockfd < 0) {
        m_lastError = QString("Failed to create socket: %1").arg(strerror(errno));
        emit errorOccurred(m_lastError);
        return false;
    }

    // Set socket to non-blocking mode
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    // Setup server address
    sockaddr_un server_addr = {};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socketPath.toUtf8().constData(),
            sizeof(server_addr.sun_path) - 1);

    // Attempt connection
    if (::connect(m_sockfd, reinterpret_cast<sockaddr*>(&server_addr),
                  sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            m_lastError = QString("Failed to connect to %1: %2")
                .arg(socketPath)
                .arg(strerror(errno));
            emit errorOccurred(m_lastError);
            ::close(m_sockfd);
            m_sockfd = -1;
            return false;
        }
    }

    // Setup socket notifier for read events
    m_socketNotifier = new QSocketNotifier(m_sockfd, QSocketNotifier::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated,
            this, &GuiIpcClient::onSocketReadyRead);

    m_connected = true;
    m_readBuffer.clear();

    qDebug() << "GUI IPC Client: Connected to" << socketPath;
    emit connected();

    return true;
}

void GuiIpcClient::closeConnection()
{
    if (m_socketNotifier) {
        m_socketNotifier->setEnabled(false);
        m_socketNotifier->deleteLater();
        m_socketNotifier = nullptr;
    }

    if (m_sockfd >= 0) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }

    if (m_connected) {
        m_connected = false;
        qDebug() << "GUI IPC Client: Disconnected";
        emit disconnected();
    }

    m_readBuffer.clear();
}

void GuiIpcClient::scheduleReconnect()
{
    if (!m_reconnectTimer->isActive()) {
        qDebug() << "GUI IPC Client: Scheduling reconnect in" << m_reconnectDelay << "ms";
        m_reconnectTimer->start(m_reconnectDelay);

        // Exponential backoff
        m_reconnectDelay = qMin(m_reconnectDelay * 2, MAX_RECONNECT_DELAY);
    }
}

void GuiIpcClient::attemptReconnect()
{
    qDebug() << "GUI IPC Client: Attempting reconnection...";
    if (!createConnection(m_socketPath)) {
        scheduleReconnect();
    }
}

void GuiIpcClient::onSocketReadyRead()
{
    // Read available data
    char buffer[4096];
    ssize_t bytesRead = ::read(m_sockfd, buffer, sizeof(buffer));

    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return; // No data available
        }
        m_lastError = QString("Read error: %1").arg(strerror(errno));
        emit errorOccurred(m_lastError);
        closeConnection();
        scheduleReconnect();
        return;
    }

    if (bytesRead == 0) {
        // Connection closed by server
        qDebug() << "GUI IPC Client: Server closed connection";
        closeConnection();
        scheduleReconnect();
        return;
    }

    // Append to buffer
    m_readBuffer.append(buffer, bytesRead);

    // Process complete frames
    while (m_readBuffer.size() >= FRAME_SIZE) {
        // Extract one frame
        qpmu::Measurement_Frame frame;
        memcpy(&frame, m_readBuffer.constData(), FRAME_SIZE);

        // Remove processed frame from buffer
        m_readBuffer.remove(0, FRAME_SIZE);

        // Emit signal with frame
        emit frameReceived(frame);
    }
}
