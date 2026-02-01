#pragma once
#include <string>
#include "EventLoop.h"
#include "Acceptor.h"
#include "utils/NonCopyable.h"

class TcpServer : public NonCopyable
{
public:
    TcpServer(std::string const& _ip, unsigned short _port);
    ~TcpServer();

    /**
     * @brief start the TCP server to accept connections.
     *
     */
    void start();

    /**
     * @brief stop the TCP server.
     *
     */
    void stop();

    /**
     * @brief set all callback functions for TcpConnection events.
     *
     * @param _onConnection Callback function for new connection events.
     * @param _onMessage Callback function for message readable events.
     * @param _onClose Callback function for connection closed events.
     */
    void setAllCallbacks(TcpConnectionCallback&& _onConnection,
                         TcpConnectionCallback&& _onMessage,
                         TcpConnectionCallback&& _onClose);

private:
    Acceptor m_acceptor;
    EventLoop m_eventLoop;
};