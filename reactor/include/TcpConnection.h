#pragma once
#include <memory>
#include <string>
#include <functional>
#include "Socket.h"
#include "SocketIO.h"
#include "InetAddress.h"
#include "utils/NonCopyable.h"

class EventLoop;
class TcpConnection;
using TcpConnectionCallback = std::function<void(std::shared_ptr<TcpConnection> const&)>;

class TcpConnection : public NonCopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    explicit TcpConnection(EventLoop* _loop, int _fd);
    ~TcpConnection();

    /**
     * @brief  Check if the connection is closed.
     *
     * @return bool True if the connection is closed, false otherwise.
     */
    bool isClosed() const;

    /**
     * @brief Receive a message from the connected socket.
     *
     * @return std::string The received message.
     */
    std::string recive();

    /**
     * @brief Send a message to the connected socket.
     *
     * @param _msg Message to send.
     */
    void send(std::string const& _msg);

    /**
     * @brief The local and peer addresses conversion to string to debug.
     *
     * @return std::string Representation of local and peer addresses.
     */
    std::string toString() const;

    /**
     * @brief Set the callback functions for new connection events.
     *
     * @param _cb The callback function.
     */
    void setOnConnectionCallback(TcpConnectionCallback const& _cb);

    /**
     * @brief Set the callback functions for event of connfd readable.
     *
     * @param _cb The callback function.
     */
    void setOnMessageCallback(TcpConnectionCallback const& _cb);

    /**
     * @brief Set the callback functions for connection closed events.
     *
     * @param _cb The callback function.
     */
    void setOnCloseCallback(TcpConnectionCallback const& _cb);

    /**
     * @brief Execute the connection callback when a new connection is established.
     *
     */
    void handleConnectionCallback();

    /**
     * @brief Execute the message callback when connection is readable.
     *
     */
    void handleMessageCallback();

    /**
     * @brief Execute the close callback when connection is closed.
     *
     */
    void handleCloseCallback();

    /**
     * @brief send the processed results to the EventLoop.
     *
     * @param msg Processed results.
     */
    void sendInLoop(std::string const& _sendMsg);

private:
    /**
     * @brief Get the local address of the listening socket.
     *
     * @return const InetAddress Listening socket address.
     */
    InetAddress const getLocalAddr() const;

    /**
     * @brief Get the peer address of the connected socket.
     *
     * @return const InetAddress Connected socket address.
     */
    InetAddress const getPeerAddr() const;

private:
    /**
     * @brief Used to send the processed results to the EventLoop.
     *
     * @details The EventLoop is responsible for communicating with the client.
     *
     */
    EventLoop* m_loop;

    SocketIO m_sockio;

    /* for Debug */
    /**
     * @brief The connected socket.
     *
     */
    Socket m_sock;
    /**
     * @brief The local address of the listening socket.
     *
     */
    InetAddress m_localAddr;
    /**
     * @brief accepted connection's peer address.
     *
     */
    InetAddress m_peerAddr;

    /**
     * @brief Callback functions for new connection events.
     *
     */
    TcpConnectionCallback m_onConnection;

    /**
     * @brief Callback functions for event of connfd readable.
     *
     */
    TcpConnectionCallback m_onMessage;

    /**
     * @brief Callback functions for connection closed events.
     *
     */
    TcpConnectionCallback m_onClose;
};