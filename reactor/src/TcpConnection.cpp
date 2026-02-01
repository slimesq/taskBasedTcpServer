#include "TcpConnection.h"
#include "EventLoop.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include "InetAddress.h"
#include <cstdio>
#include <iostream>
#include <sstream>

TcpConnection::TcpConnection(EventLoop* _loop, int _fd)
    : m_loop(_loop),
      m_sockio(_fd),
      m_sock(_fd),
      m_localAddr(getLocalAddr()),
      m_peerAddr(getPeerAddr())
{
}

TcpConnection::~TcpConnection()
{
}

bool TcpConnection::isClosed() const
{
    char buf[10] = {};
    int ret = ::recv(this->m_sock.getFd(), buf, sizeof(buf), MSG_PEEK);
    if (ret == 0)
    {
        return true;
    }
    return false;
}

std::string TcpConnection::recive()
{
    char buf[4096]{0};
    this->m_sockio.readline(buf, sizeof(buf));
    return std::string(buf);
}

void TcpConnection::send(std::string const& _msg)
{
    this->m_sockio.writen(_msg.c_str(), _msg.size());
}

std::string TcpConnection::toString() const
{
    std::ostringstream oss;
    oss << this->m_localAddr.getIp() << ":" << this->m_localAddr.getPort() << " ----> "
        << this->m_peerAddr.getIp() << ":" << this->m_peerAddr.getPort();
    return oss.str();
}

void TcpConnection::setOnConnectionCallback(TcpConnectionCallback const& _cb)
{
    this->m_onConnection = _cb;
}

void TcpConnection::setOnMessageCallback(TcpConnectionCallback const& _cb)
{
    this->m_onMessage = _cb;
}

void TcpConnection::setOnCloseCallback(TcpConnectionCallback const& _cb)
{
    this->m_onClose = _cb;
}

void TcpConnection::handleConnectionCallback()
{
    if (this->m_onConnection)
    {
        /*It is not possible to directly pass the 'this' pointer. Doing so would result in creating
         * a temporary shared_ptr to hold the 'this' pointer, which could lead to double free
         * errors.*/
        // this->m_onConnection(std::shared_ptr<TcpConnection>(this)); // error
        this->m_onConnection(shared_from_this());
    }
    else
    {
        std::cout << "TcpConnection::handleConnectionCallback: onConnection is not set"
                  << std::endl;
        return;
    }
}

void TcpConnection::handleMessageCallback()
{
    if (this->m_onMessage)
    {
        // this->m_onMessage(std::shared_ptr<TcpConnection>(this));    // error
        this->m_onMessage(shared_from_this());
    }
    else
    {
        std::cout << "TcpConnection::handleMessageCallback: onMessage is not set" << std::endl;
        return;
    }
}

void TcpConnection::handleCloseCallback()
{
    if (this->m_onClose)
    {
        // this->m_onClose(std::shared_ptr<TcpConnection>(this));  // error
        this->m_onClose(shared_from_this());
    }
    else
    {
        std::cout << "TcpConnection::handleCloseCallback: onClose is not set" << std::endl;
        return;
    }
}

InetAddress const TcpConnection::getLocalAddr() const
{
    // The local address corresponding to sockfd can be stored in the second parameter.
    struct sockaddr_in localAddr;
    socklen_t addrlen = sizeof(localAddr);
    int ret{::getsockname(m_sock.getFd(), (struct sockaddr*)&localAddr, &addrlen)};
    if (ret == -1)
    {
        ::perror("getsockname error");
    }

    return InetAddress(localAddr);
}

InetAddress const TcpConnection::getPeerAddr() const
{
    // The address of the opposite end corresponding to sockfd can be stored in the second
    // parameter.
    struct sockaddr_in peerAddr;
    socklen_t addrlen = sizeof(peerAddr);
    int ret{::getpeername(m_sock.getFd(), (struct sockaddr*)&peerAddr, &addrlen)};
    if (ret == -1)
    {
        ::perror("getpeername error");
    }
    return InetAddress(peerAddr);
}

/**
 * @details In the function of "sendInLoop" here, the parameter "msg" is the data that has been
 * processed by the thread pool. After that, it can be sent to the Reactor/EventLoop for further
 * processing.
 * However, for the Reactor/EventLoop, even if they obtain the processed data "msg",
 * they cannot send the data to the client because for the Reactor/EventLoop, it does not have the
 * ability to send data by itself.
 *
 * In the entire design, there are two classes that have the ability to send data. One
 * is TcpConnection, and the other is SocketIO. However, SocketIO is more focused on
 * the lower level, and the actual object that truly sends data externally is the TcpConnection
 * object, or more specifically, the send function within TcpConnection. Therefore, the send
 * function of TcpConnection, along with the data to be sent (msg), need to be packaged together as
 * a "task" and handed over to the EventLoop/Reactor. In this way, the EventLoop/Reactor can send
 * the data to the client.
 *
 */
void TcpConnection::sendInLoop(std::string const& _sendMsg)
{
    std::function<void()> f = std::bind(&TcpConnection::send, this, _sendMsg);
    if (this->m_loop)
    {
        this->m_loop->runInLoop(std::move(f));
    }
}