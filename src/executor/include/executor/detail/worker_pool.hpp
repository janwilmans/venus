/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/task.hpp"

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace venus {

/**
 * @brief The number of threads that can run concurrently on this system, at least 1.
 */
[[nodiscard]] std::size_t default_thread_count();

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
     * Must not be called from a task running on this pool, that would wait for itself.
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

} // namespace detail
} // namespace venus
