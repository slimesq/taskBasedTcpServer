#include <Socket.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <cstdio>
#include <cstdlib>

Socket::Socket()
{
    this->m_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (this->m_fd == -1)
    {
        ::perror("socket create error");
    }
}

Socket::Socket(int _fd) : m_fd(_fd)
{
}

Socket::~Socket()
{
    close(this->m_fd);
}

int Socket::getFd() const noexcept
{
    return this->m_fd;
}