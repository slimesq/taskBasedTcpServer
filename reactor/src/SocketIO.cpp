#include "SocketIO.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

SocketIO::SocketIO(int _fd) : m_fd(_fd)
{
}

SocketIO::~SocketIO()
{
}

ssize_t SocketIO::readn(void* _buf, size_t _count)
{
    size_t nleft = _count;
    ssize_t nread = 0;
    char* pstr = static_cast<char*>(_buf);

    while (nleft > 0)
    {
        nread = ::read(this->m_fd, pstr, nleft);
        if (nread < 0)
        {
            // Handle interrupted system calls
            if (errno == EINTR)
                continue;
            else
            {
                ::perror("read error");
                ::exit(-1);
            }
        }
        else if (nread == 0)
        {
            break;
        }
        pstr += nread;
        nleft -= nread;
    }

    return _count - nleft;
}

ssize_t SocketIO::writen(void const* _buf, size_t _count)
{
    size_t nleft = _count;
    ssize_t nwritten = 0;
    char const* pstr = static_cast<char const*>(_buf);

    while (nleft > 0)
    {
        nwritten = ::write(this->m_fd, pstr, nleft);
        if (nwritten < 0)
        {
            // Handle interrupted system calls
            if (errno == EINTR)
                continue;
            else
            {
                ::perror("write error");
                return -1;
            }
        }
        pstr += nwritten;
        nleft -= nwritten;
    }

    return _count - nleft;
}

ssize_t SocketIO::readline(void* _buf, size_t _maxCount)
{
    size_t left = _maxCount - 1;
    char* pstr = static_cast<char*>(_buf);
    int ret = 0;
    ssize_t total = 0;

    while (left > 0)
    {
        // MSG_PEEK does not clear the data in the buffer, it only copies it.
        ret = ::recv(this->m_fd, pstr, left, MSG_PEEK);
        if (ret == -1 && errno == EINTR)
        {
            continue;
        }
        else if (ret == -1)
        {
            ::perror("readLine error");
            ::exit(1);
        }
        else if (ret == 0)
        {
            break;
        }
        else
        {
            for (int idx = 0; idx < ret; ++idx)
            {
                if (pstr[idx] == '\n')
                {
                    int sz = idx + 1;
                    this->readn(pstr, sz);
                    pstr += sz;
                    *pstr = '\0';       // c style string ends with '\0'
                    return total + sz;  // return total bytes read.
                }
            }
            this->readn(pstr, ret);  // copy from kernel buffer to user buffer.
            total += ret;
            pstr += ret;
            left -= ret;
        }
    }
    *pstr = '\0';
    return total;  // return total bytes read.
}
