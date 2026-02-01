#pragma once
#include "Task.h"

class MyTask : public Task
{
public:
    MyTask(std::shared_ptr<TcpConnection> const& _conn, const std::string& _recvMsg);
    ~MyTask() override;

    /**
     * @brief Override the process function to define task behavior.
     *
     * @details Tasks need to be added based on the specific business logic.
     *
     */
    void process() override;
};