#pragma once

#include <pthread.h>
#include <cstddef>
#include <vector>
#include <thread>
#include "TaskQueue.h"
#include "utils/NonCopyable.h"

class Task;

class ThreadPool : public NonCopyable
{
public:
    ThreadPool(size_t _threadNum, size_t _queueSize);
    ~ThreadPool();

    /**
     * @brief start the thread pool.
     *
     */
    void start();

    /**
     * @brief stop the thread pool.
     *
     */
    void stop();

    /**
     * @brief add a task into the task queue.
     *
     * @param _pTask The task pointer.
     */
    void addTask(TaskFunc _task);

    /**
     * @brief get a task from the task queue.
     *
     * @return TaskPtr the task pointer.
     */
    TaskFunc getTask();

private:
    /**
     * @brief The tasks assigned to the worker threads by the thread pool
     *
     */
    void doTask();

private:
    /**
     * @brief number of worker threads in the thread pool.
     *
     */
    size_t m_threadNum;

    /**
     * @brief storage for child threads.
     *
     */
    std::vector<std::thread> m_threads;

    /**
     * @brief task queue size.
     *
     */
    size_t m_queueSize;

    /**
     * @brief task queue.
     *
     */
    TaskQueue m_taskQueue;

    /**
     * @brief indicate if the thread pool is exiting.
     *
     */
    bool m_isExit;
};