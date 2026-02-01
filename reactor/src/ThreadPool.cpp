#include "ThreadPool.h"
#include <unistd.h>
#include <iostream>
#include "Task.h"
#include "TaskQueue.h"

ThreadPool::ThreadPool(size_t _threadNum, size_t _queueSize)
    : m_threadNum(_threadNum),
      m_threads(),
      m_queueSize(_queueSize),
      m_taskQueue(m_queueSize),
      m_isExit(false)
{
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::start()
{
    /* create worker threads(After a thread is created, it needs to execute the thread entry
    function.)*/
    for (size_t i = 0; i != this->m_threadNum; ++i)
    {
        std::thread th(&ThreadPool::doTask, this);
        this->m_threads.push_back(std::move(th));
    }
}

void ThreadPool::stop()
{
    // If the task is not completed, the child thread must not be allowed to exit.
    while (!this->m_taskQueue.empty())
    {
        ::usleep(5000);  // main thread sleep 1 second and check again.
    }

    this->m_isExit = true;

    // Wake up all the worker threads before recycling.
    this->m_taskQueue.wakeAll();

    for (auto& th : this->m_threads)
    {
        // Recycling work thread.
        th.join();
    }
}

void ThreadPool::addTask(TaskFunc _task)
{
    if (_task)
    {
        this->m_taskQueue.push(_task);
    }
}

TaskFunc ThreadPool::getTask()
{
    return this->m_taskQueue.pop();
}

void ThreadPool::doTask()
{
    /* As long as the task queue is not empty, the worker thread should be made to acquire and
     execute the task.*/
    while (!this->m_isExit)
    {
        // get a task from the task queue.
        TaskFunc task = this->getTask();
        if (task)
        {
            // execute the task.
            task();  // This part will demonstrate polymorphism.
        }
        else
        {
            std::cout << "ThreadPool: no task to process, thread id = "
                      << std::this_thread::get_id() << std::endl;
        }
    }
}