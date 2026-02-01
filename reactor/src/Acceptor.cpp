#include "Acceptor.h"
#include <asm-generic/socket.h>
#include <sys/socket.h>

Acceptor::Acceptor(std::string const& _ip, unsigned short _port) : m_sock(), m_addr(_ip, _port)
{
}

Acceptor::~Acceptor()
{
}

int Acceptor::getFd() const
{
    return this->m_sock.getFd();
}

int Acceptor::accept()
{
    int connfd = ::accept(this->m_sock.getFd(), nullptr, nullptr);
    if (connfd == -1)
    {
        perror("accept error");
        ::exit(1);
    }
    return connfd;
}

void Acceptor::ready()
{
    setReuseAddr();
    setReusePort();
    bind();
    listen();
}

void Acceptor::setReuseAddr()
{
    int opt{1};
    int ret{::setsockopt(this->m_sock.getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))};
    if (ret == -1)
    {
        ::perror("setsockopt SO_REUSEADDR error");
        ::exit(1);
    }
}

void Acceptor::setReusePort()
{
    int opt{1};
    int ret{::setsockopt(this->m_sock.getFd(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))};
    if (ret == -1)
    {
        ::perror("setsockopt SO_REUSEPORT error");
        ::exit(1);
    }
}

void Acceptor::bind()
{
    int ret{::bind(this->m_sock.getFd(),
                   (struct sockaddr*)this->m_addr.getInetAddressPtr(),
                   sizeof(struct sockaddr_in))};
    if (ret == -1)
    {
        perror("bind error");
        ::exit(1);
    }
}

void Acceptor::listen()
{
    int ret{::listen(this->m_sock.getFd(), 128)};
    if (ret == -1)
    {
        ::perror("listen error");
        ::exit(1);
    }
}