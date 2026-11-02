/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/task.hpp"

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace venus {

/**
 * @brief The number of threads that can run concurrently on this system, at least 1.
 */
[[nodiscard]] inline std::size_t default_thread_count()
{
    return std::max<std::size_t>(1, std::thread::hardware_concurrency());
}

namespace detail {

/**
 * @brief A fixed set of worker threads that execute tasks from a multi-level queue.
 *
 * There is one FIFO queue per level, level 0 has the highest priority. A worker always takes the
 * oldest task of the highest priority level that has work. All levels are protected by one mutex,
 * which keeps priorities exact and guarantees that no task waits while a worker is idle.
 *
 * Tasks run in parallel; the pool itself gives no ordering guarantees between tasks of the same level
 * other than that they are _started_ in FIFO order.
 */
class worker_pool
{
public:
    worker_pool(std::size_t thread_count, std::size_t level_count);

    /**
     * @brief Waits until all tasks are finished (including tasks added by tasks), then stops the workers.
     *
     * Note: adding tasks from other threads during destruction violates the C++ object lifetime rules.
     */
    ~worker_pool();

    worker_pool(const worker_pool &) = delete;
    worker_pool & operator=(const worker_pool &) = delete;
    worker_pool(worker_pool &&) = delete;
    worker_pool & operator=(worker_pool &&) = delete;

    void push(std::size_t level, task t);

    /**
     * @brief Blocks until there are no queued or running tasks.
     *
     * @throws std::logic_error when called from a task running on this pool, it would wait for itself.
     */
    void wait_idle();

    [[nodiscard]] std::size_t thread_count() const;
    [[nodiscard]] std::size_t level_count() const;

    /**
     * @brief Checks if the calling thread is one of the workers of this pool.
     */
    [[nodiscard]] bool is_worker_thread() const;

private:
    void run();
    void stop_and_join();
    [[nodiscard]] bool has_work() const;
    task pop();

    std::mutex m_mutex;
    std::condition_variable m_work_available;
    std::condition_variable m_idle;
    std::vector<std::deque<task>> m_levels;
    std::size_t m_pending = 0;  // tasks queued or running
    std::size_t m_sleeping = 0; // workers waiting for m_work_available
    bool m_stop = false;
    std::vector<std::thread> m_threads;
};

// implementation

// the pool the current thread works for, nullptr on threads that are not workers
// (a function local static in an inline function is one object per thread for the whole program)
inline const worker_pool *& current_pool()
{
    static thread_local const worker_pool * pool = nullptr;
    return pool;
}

inline worker_pool::worker_pool(std::size_t thread_count, std::size_t level_count) :
    m_levels(std::max<std::size_t>(1, level_count))
{
    const auto count = std::max<std::size_t>(1, thread_count);
    m_threads.reserve(count);
    try
    {
        std::generate_n(std::back_inserter(m_threads), count, [this] { return std::thread([this] { run(); }); });
    }
    catch (...)
    {
        // creating a thread failed, stop the threads that did start before the exception leaves the constructor
        stop_and_join();
        throw;
    }
}

inline worker_pool::~worker_pool()
{
    // wait_idle() throws when called from a worker, from a noexcept destructor that terminates instead of deadlocking
    wait_idle();
    stop_and_join();
}

inline void worker_pool::stop_and_join()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_work_available.notify_all();
    for (auto & thread : m_threads)
    {
        thread.join();
    }
}

inline void worker_pool::push(std::size_t level, task t)
{
    assert(level < m_levels.size());
    bool wake = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_levels[level].push_back(std::move(t));
        ++m_pending;
        // when all workers are busy, a worker will find this task when it finishes, notifying would only cost time
        wake = m_sleeping > 0;
    }

    if (wake)
    {
        m_work_available.notify_one();
    }
}

inline void worker_pool::wait_idle()
{
    if (is_worker_thread())
    {
        throw std::logic_error("venus: wait_idle() called from a task on the same pool, this would deadlock");
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle.wait(lock, [this] { return m_pending == 0; });
}

inline std::size_t worker_pool::thread_count() const
{
    return m_threads.size();
}

inline std::size_t worker_pool::level_count() const
{
    return m_levels.size();
}

inline bool worker_pool::is_worker_thread() const
{
    return current_pool() == this;
}

inline bool worker_pool::has_work() const
{
    return std::any_of(m_levels.begin(), m_levels.end(), [](const std::deque<task> & level) { return !level.empty(); });
}

inline task worker_pool::pop()
{
    auto level = std::find_if(m_levels.begin(), m_levels.end(), [](const std::deque<task> & queue) { return !queue.empty(); });
    assert(level != m_levels.end());
    task result = std::move(level->front());
    level->pop_front();
    return result;
}

inline void worker_pool::run()
{
    current_pool() = this;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;)
    {
        while (!m_stop && !has_work())
        {
            ++m_sleeping;
            m_work_available.wait(lock);
            --m_sleeping;
        }

        if (!has_work())
        {
            return; // stopped and no work left
        }

        {
            task current = pop();
            lock.unlock();
            try
            {
                current();
            }
            catch (...)
            {
                // exceptions from call_async() style tasks are delivered through their futures, others are ignored.
            }
            // 'current' is destroyed here, without holding the lock, its captures may run arbitrary destructors
        }

        lock.lock();
        if (--m_pending == 0)
        {
            m_idle.notify_all();
        }
    }
}

} // namespace detail
} // namespace venus
