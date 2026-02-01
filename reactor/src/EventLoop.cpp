#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <memory>
#include <sys/eventfd.h>
#include <iostream>
#include <mutex>
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Acceptor.h"

EventLoop::EventLoop(Acceptor& _acceptor)
    : m_epfd(createEpollFd()),
      m_evtList(1024),
      m_isLooping(false),
      m_acceptor(_acceptor),
      m_evtfd(createEventFd()),
      m_pengdings(),
      m_mutex()
{
    // Place the listenfd on the red-black tree for monitoring.
    addEpollReadFd(this->m_acceptor.getFd());

    // Place the communication used between threads on a red-black tree for monitoring.
    addEpollReadFd(this->m_evtfd);
}

EventLoop::~EventLoop()
{
    ::close(m_epfd);
}

void EventLoop::loop()
{
    this->m_isLooping = true;
    while (this->m_isLooping)
    {
        waitEpollFd();
    }
}

void EventLoop::unloop()
{
    this->m_isLooping = false;
}

void EventLoop::waitEpollFd()
{
    int nready{0};
    do
    {
        nready = ::epoll_wait(
            this->m_epfd, &(this->m_evtList[0]), static_cast<int>(this->m_evtList.size()), 3000);

    } while (nready == -1 && errno == EINTR);

    if (nready == -1)
    {
        // interrupted by signal, just ignore.
        ::perror("epoll_wait error");
        return;
    }
    else if (nready == 0)
    {
        // timeout, no events happened.
        // std::cout << "epoll_wait timeout!" << std::endl;
        return;
    }
    else
    {
        // If all events are used, resize the event list to double its size.
        if (nready == static_cast<int>(this->m_evtList.size()))
        {
            this->m_evtList.resize(this->m_evtList.size() * 2);
        }

        // handle all ready events.
        for (int i{0}; i < nready; ++i)
        {
            int curFd{this->m_evtList[i].data.fd};
            if (curFd == this->m_acceptor.getFd())
            {
                handleNewConnection();
            }
            else if (curFd == this->m_evtfd)  // The file descriptor for communication is now ready.
            {
                this->handleEventFdRead();

                /* do the assignment */

                // Traverse m_pengdings
                this->doPengdingFunctors();
            }
            else
            {
                handleMessage(curFd);
            }
        }
    }
}

void EventLoop::handleNewConnection()
{
    int connfd{this->m_acceptor.accept()};
    if (connfd < 0)
    {
        ::perror("handleNewConnection accept error");
        return;
    }

    /* The three-way handshake was successfully established, and connfd is placed on the red-black
     tree for listening.*/
    addEpollReadFd(connfd);

    /*Create a TcpConnection for the new connection and store it in the connection map.*/
    std::shared_ptr<TcpConnection> tcpConn{std::make_shared<TcpConnection>(this, connfd)};
    // this->m_conns.insert(std::make_pair(connfd, tcpConn));
    this->m_conns[connfd] = tcpConn;
    std::cout << "new connection established: " << tcpConn->toString() << std::endl;

    /*
     *set three callback functions for the new connection.
     *
     *three callback functions are set in EventLoop, and then passed to TcpConnection.
     */
    tcpConn->setOnConnectionCallback(this->m_onConnection);
    tcpConn->setOnMessageCallback(this->m_onMessage);
    tcpConn->setOnCloseCallback(this->m_onClose);

    /* connection established, call the connection callback function. */
    tcpConn->handleConnectionCallback();
}

/* handle old connections */
void EventLoop::handleMessage(int _fd)
{
    if (auto it{this->m_conns.find(_fd)}; it != this->m_conns.end())
    {
        bool flag = it->second->isClosed();
        if (flag)
        {
            // connection closed, call the close callback function.
            it->second->handleCloseCallback();
            // delete the fd from epoll read set.
            delEpollReadFd(_fd);
            // remove the connection from the connection map.
            this->m_conns.erase(it);
        }
        else
        {
            // connection readable, call the message callback function.
            it->second->handleMessageCallback();
        }
    }
    else
    {
        ::perror("handleMessage:connection is not found");
        return;
    }
}

int EventLoop::createEpollFd()
{
    int epfd{::epoll_create(1)};
    if (epfd == -1)
    {
        ::perror("epoll_create error");
        return -1;
    }
    return epfd;
}

void EventLoop::addEpollReadFd(int _fd)
{
    struct epoll_event event;
    event.data.fd = _fd;
    event.events = EPOLLIN;
    int ret{::epoll_ctl(this->m_epfd, EPOLL_CTL_ADD, _fd, &event)};
    if (ret == -1)
    {
        ::perror("epoll_ctl add error");
        return;
    }
    return;
}

void EventLoop::modEpollReadFd(int _fd, struct epoll_event& _event)
{
    int ret{::epoll_ctl(this->m_epfd, EPOLL_CTL_MOD, _fd, &_event)};
    if (ret == -1)
    {
        ::perror("epoll_ctl mod error");
        return;
    }
}

void EventLoop::delEpollReadFd(int _fd)
{
    int ret{::epoll_ctl(this->m_epfd, EPOLL_CTL_DEL, _fd, nullptr)};
    if (ret == -1)
    {
        ::perror("epoll_ctl del error");
        return;
    }
}

void EventLoop::setOnConnectionCallback(TcpConnectionCallback&& _cb)
{
    this->m_onConnection = std::move(_cb);
}

void EventLoop::setOnMessageCallback(TcpConnectionCallback&& _cb)
{
    this->m_onMessage = std::move(_cb);
}

void EventLoop::setOnCloseCallback(TcpConnectionCallback&& _cb)
{
    this->m_onClose = std::move(_cb);
}

int EventLoop::createEventFd()
{
    int evtfd{::eventfd(0, 0)};
    if (evtfd == -1)
    {
        ::perror("eventfd error");
        return -1;
    }
    return evtfd;
}

void EventLoop::handleEventFdRead()
{
    uint64_t one{1};
    ssize_t ret{::read(this->m_evtfd, &one, sizeof(uint64_t))};
    if (ret == -1)
    {
        ::perror("handleEventFdRead error");
        return;
    }
}

void EventLoop::wakeupEventFd()
{
    uint64_t one{1};
    ssize_t ret{::write(this->m_evtfd, &one, sizeof(uint64_t))};
    if (ret == -1)
    {
        ::perror("handleEventFdRead error");
        return;
    }
}

void EventLoop::doPengdingFunctors()
{
    std::lock_guard<std::mutex> lock(this->m_mutex);
    std::vector<Functors> tmp;
    tmp.swap(this->m_pengdings);

    for (auto& cb : tmp)
    {
        cb();
    }
}

void EventLoop::runInLoop(Functors&& _cb)
{
    {  // The granularity of the control lock
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_pengdings.push_back(std::move(_cb));
    }

    this->wakeupEventFd();
    // std::cout << "wakeupEventFd" << std::endl;
}