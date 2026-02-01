#pragma once

#include <condition_variable>
#include <functional>
#include <queue>
#include "utils/NonCopyable.h"
#include "Task.h"

using TaskFunc = std::function<void()>;

class TaskQueue : public NonCopyable
{
public:
    TaskQueue(size_t _size);
    ~TaskQueue();

    /**
     * @brief add a task into the queue.
     *
     * @param _task
     */
    void push(TaskFunc _task);

    /**
     * @brief get a task from the queue.
     *
     * @return TaskPtr The task pointer.
     */
    TaskFunc pop();

    /**
     * @brief check if the queue is empty.
     *
     * @return true if the queue is empty, false otherwise.
     */
    bool empty() const;

    /**
     * @brief check if the queue is full.
     *
     * @return true if the queue is full, false otherwise.
     */
    bool full() const;

    /**
     * @brief wake up all waiting threads.
     *
     * @details Before the thread pool is terminated, wake up the threads that are blocked due to
     * the task queue being empty.
     *
     */
    void wakeAll();

private:
    /**
     * @brief task queue size limit.
     *
     */
    size_t m_queueSize;

    /**
     * @brief storage for tasks.
     *
     */
    std::queue<TaskFunc> m_queue;

    /**
     * @brief mutex for thread safety.
     *
     */
    std::mutex m_mutex;

    /**
     * @brief condition variable for not empty.
     *
     */
    std::condition_variable m_notEmpty;

    /**
     * @brief condition variable for not full.
     *
     */
    std::condition_variable m_notFull;

    /**
     * @brief indicate if the task queue is running.prevent threads from waiting forever.
     *
     */
    bool m_isRunning;
};