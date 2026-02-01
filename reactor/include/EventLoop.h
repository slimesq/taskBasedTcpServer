#pragma once
#include <functional>
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include "TcpConnection.h"
#include <sys/epoll.h>

class Acceptor;

using Functors = std::function<void()>;

class EventLoop
{
public:
    EventLoop(Acceptor& _acceptor);
    ~EventLoop();

    /**
     * @brief loop to call waitEpollFd to wait for events.
     *
     */
    void loop();

    /**
     * @brief stop the event loop.
     *
     */
    void unloop();

    /**
     * @brief Set the callback functions for new connection events.
     *
     * @param _cb The callback function.
     */
    void setOnConnectionCallback(TcpConnectionCallback&& _cb);

    /**
     * @brief Set the callback functions for event of connfd readable.
     *
     * @param _cb The callback function.
     */
    void setOnMessageCallback(TcpConnectionCallback&& _cb);

    /**
     * @brief Set the callback functions for connection closed events.
     *
     * @param _cb The callback function.
     */
    void setOnCloseCallback(TcpConnectionCallback&& _cb);

    /**
     * @brief create event fd.
     *
     * @return int fd.
     */
    int createEventFd();

    /**
     * @brief Read data from the m_evtfd kernel counter and clear the kernel counter.
     *
     */
    void handleEventFdRead();

    /**
     * @brief write data from the m_evtfd kernel counter and add the kernel counter.
     *
     */
    void wakeupEventFd();

    /**
     * @brief  Traverse m_pengdings
     *
     */
    void doPengdingFunctors();

    /**
     * @brief Store "task" in the m_pengdings and wakeup the eventfd
     *
     * @param _cb task.
     */
    void runInLoop(Functors&& _cb);

private:
    /**
     * @brief wait for events on epoll fd and handle them.
     *
     */
    void waitEpollFd();

    /**
     * @brief handle new connection event.
     *
     */
    void handleNewConnection();

    /**
     * @brief handle old connection event.
     *
     * @param _fd The file descriptor of the old connection.
     */
    void handleMessage(int _fd);

    /**
     * @brief create epoll fd.
     *
     * @return int The new file descriptor of epoll fd.
     */
    int createEpollFd();

    /**
     * @brief add a fd to Red-black tree in epoll read set.
     *
     * @param _fd The file descriptor to add.
     */
    void addEpollReadFd(int _fd);

    /**
     * @brief modify a fd in epoll read set using the given event.
     *
     * @param _fd The file descriptor to modify.
     * @param _event The epoll event.
     */
    void modEpollReadFd(int _fd, struct epoll_event& _event);

    /**
     * @brief delete a fd from Red-black tree in epoll read set.
     *
     * @param _fd The file descriptor to delete.
     */
    void delEpollReadFd(int _fd);

private:
    /**
     * @brief The epoll file descriptor.
     *
     */
    int m_epfd;

    /**
     * @brief The event list to store epoll events for ready fds.
     *
     */
    std::vector<struct epoll_event> m_evtList;

    /**
     * @brief Indicates whether the event loop is running.
     *
     */
    bool m_isLooping;

    /**
     * @brief The acceptor to listen for new connections.
     *
     */
    Acceptor& m_acceptor;

    /**
     * @brief Map of file descriptors to their corresponding TcpConnection.
     *
     */
    std::map<int, std::shared_ptr<TcpConnection>> m_conns;

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

    /**
     * @brief event fd for communication with the thread pool
     *
     */
    int m_evtfd;

    /**
     * @brief Store multiple "tasks" to be executed
     *
     */
    std::vector<Functors> m_pengdings;

    /**
     * @brief Used for mutually exclusive access to m_pending.
     *
     */
    std::mutex m_mutex;
};