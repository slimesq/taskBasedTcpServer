#pragma once

#include <memory>
#include "TcpConnection.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "utils/NonCopyable.h"

using TaskFactory =
    std::function<std::shared_ptr<Task>(std::shared_ptr<TcpConnection>, std::string)>;

class TaskBasedTcpServer : public NonCopyable
{
public:
    TaskBasedTcpServer(size_t _threadNum, size_t _queSize, std::string const& _ip, unsigned short _port);
    ~TaskBasedTcpServer();

    /**
     * @brief
     *
     */
    void start();
    void stop();

    void onNewConnection(std::shared_ptr<TcpConnection> const& _conn);
    void onMessage(std::shared_ptr<TcpConnection> const& _conn);
    void onClose(std::shared_ptr<TcpConnection> const& _conn);

    void setTaskFactory(TaskFactory _factory);

private:
    ThreadPool m_pool;
    TcpServer m_server;

    /**
     * @brief 
     * 
     */
    TaskFactory m_taskFactory;
};