#include "Task.h"

Task::Task(std::shared_ptr<TcpConnection> const& _conn, std::string const& _msg)
    : m_conn(_conn), m_recvMsg(_msg)
{
}
Task::Task(Task const& _task)
{
    if (this != &_task)
    {
        this->m_conn = _task.m_conn;
        this->m_recvMsg = _task.m_recvMsg;
    }
}

Task::~Task()
{
}