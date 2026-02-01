#include "TaskQueue.h"
#include <mutex>

TaskQueue::TaskQueue(size_t _size)
    : m_queueSize(_size), m_queue(), m_mutex(), m_notEmpty(), m_notFull(), m_isRunning(true)
{
}

TaskQueue::~TaskQueue()
{
}

bool TaskQueue::empty() const
{
    return this->m_queue.empty();
}

bool TaskQueue::full() const
{
    return this->m_queue.size() == this->m_queueSize;
}

void TaskQueue::push(TaskFunc _task)
{
    // 1. add lock using RAII.
    std::unique_lock<std::mutex> lock(this->m_mutex);

    // 2. check if the queue is full.
    while (this->full())  // prevent spurious wakeup
    {
        // Wait until the queue is not full.
        this->m_notFull.wait(lock);
    }
    this->m_queue.push(_task);
    this->m_notEmpty.notify_one();  // Notify one waiting thread that the queue is not empty.
    // 3. auto to unlock when out of scope.
}

TaskFunc TaskQueue::pop()
{
    // 1. add lock using RAII.
    std::unique_lock<std::mutex> lock(this->m_mutex);

    // 2. check if the queue is empty.
    while (this->empty() && this->m_isRunning)  // prevent spurious wakeup
    {
        // Wait until the queue is not empty.
        m_notEmpty.wait(lock);
    }

    if (this->m_isRunning == true)
    {
        TaskFunc task = this->m_queue.front();
        this->m_queue.pop();
        this->m_notFull.notify_one();  // Notify one waiting thread that the queue is not full.
        return task;
    }
    else
    {
        return nullptr;
    }

    // 3. auto to unlock when out of scope.
}

void TaskQueue::wakeAll()
{
    this->m_isRunning = false;
    this->m_notEmpty.notify_all();
}