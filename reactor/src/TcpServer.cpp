#include "TcpServer.h"
#include "EventLoop.h"

TcpServer::TcpServer(std::string const& _ip, unsigned short _port)
    : m_acceptor(_ip, _port), m_eventLoop(m_acceptor)
{
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    this->m_acceptor.ready();
    this->m_eventLoop.loop();
}

void TcpServer::stop()
{
    this->m_eventLoop.unloop();
}

void TcpServer::setAllCallbacks(TcpConnectionCallback&& _onConnection,
                                TcpConnectionCallback&& _onMessage,
                                TcpConnectionCallback&& _onClose)
{
    this->m_eventLoop.setOnConnectionCallback(std::move(_onConnection));
    this->m_eventLoop.setOnMessageCallback(std::move(_onMessage));
    this->m_eventLoop.setOnCloseCallback(std::move(_onClose));
}
