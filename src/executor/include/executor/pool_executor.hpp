/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/detail/config.hpp"
#include "executor/detail/worker_pool.hpp"
#include "executor/task.hpp"

#include <cstddef>
#include <future>
#include <utility>

namespace venus {

/**
 * @brief Executes independent tasks in parallel on a fixed number of threads.
 *
 * There is one queue of work; as soon as a thread is done and there is work in the queue, it is picked up.
 * Tasks are started in the order they were added, but run in parallel, so no ordering or
 * thread safety between tasks is guaranteed.
 *
 * The destructor waits until all tasks, including tasks added by tasks, have finished.
 */
class pool_executor
{
public:
    explicit pool_executor(std::size_t thread_count = default_thread_count()) :
        m_pool(thread_count, 1)
    {
    }

    /**
     * @brief Adds a task, exceptions thrown by the task are caught and ignored.
     */
    void add(task t)
    {
        m_pool.push(0, std::move(t));
    }

    /**
     * @brief Adds a task and returns a future for its result, or the exception it throws.
     */
    template <typename Fn>
    auto call_async(Fn fn)
    {
        std::packaged_task<decltype(fn())()> packaged(std::move(fn));
        auto future = packaged.get_future();
        add(std::move(packaged));
        return future;
    }

    /**
     * @brief Blocks until all tasks are finished. @throws std::logic_error when called from a task on this pool.
     */
    void wait_idle()
    {
        m_pool.wait_idle();
    }

    VENUS_NODISCARD std::size_t thread_count() const
    {
        return m_pool.thread_count();
    }

private:
    detail::worker_pool m_pool;
};

} // namespace venus
