/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/scheduled_calls.hpp"
#include "executor/synchronized_queue.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace venus {

class executor;

/**
 * @brief Handle to a call scheduled by executor::call_at(), call_after() or call_every()
 *
 * Destroying the handle does _not_ cancel the call.
 * Calling cancel() after the executor was destroyed does nothing.
 */
class scheduled_call
{
public:
    using id_t = venus::call_t::id_t;

    scheduled_call(venus::executor & executor, scheduled_call::id_t id);
    void cancel();

    [[nodiscard]] scheduled_call::id_t id() const;

private:
    std::weak_ptr<executor *> m_executor; // expires when the executor is destroyed
    scheduled_call::id_t m_id;
};

class executor
{
public:
    executor();

    /**
     * @brief Finishes all tasks that were added before destruction started, then stops the executor thread.
     *
     * Tasks added by add(), call() and call_async() before the destructor was called are executed.
     * Scheduled calls (call_at / call_after / call_every) that have not run yet are discarded.
     *
     * Note: users still using the executor during destruction are in violation of the C++ object lifetime rules.
     */
    ~executor();

    executor(const executor &) = delete;
    executor & operator=(const executor &) = delete;
    executor(executor &&) = delete;
    executor & operator=(executor &&) = delete;

    template <typename Fn>
    auto call(Fn fn)
    {
        if (is_executor_thread())
        {
            assert(false && "calling call() inside the executor thread is usually a mistake");
            return fn();
        }

        // A packaged_task encapsulates a task and its associated promise in one object.
        std::packaged_task<decltype(fn())()> task(std::move(fn));
        add([&task]() { task(); });
        return task.get_future().get();
    }

    template <typename Fn>
    auto call_async(Fn fn)
    {
        auto pTask = std::make_shared<std::packaged_task<decltype(fn())()>>(std::move(fn));
        auto f = pTask->get_future();
        add([pTask]() { (*pTask)(); });
        return f;
    }

    /**
     * @brief Checks if the calling thread is the executor's designated thread.
     *
     * @return `true` if the calling thread is the executor thread; otherwise, `false`.
     */
    [[nodiscard]] bool is_executor_thread() const;

    /**
     * @brief Executes the main loop of the executor.
     *
     * Processes tasks continuously in a loop, sleeps when no work is available.
     * This function runs on the executor's designated thread.
     */
    void run();

    /**
     * @brief Synchronizes the calling thread with the executor thread.
     *
     * Blocks the calling thread until it is synchronized with the executor thread.
     * Ensures that all previously scheduled tasks are completed, providing a consistent
     * state between the executor thread and the calling thread.
     *
     * @throws std::logic_error when called on the executor thread, waiting for itself would deadlock.
     */
    void synchronize();

    /**
     * @brief Adds a task to be executed as soon as possible, in order.
     *
     * Exceptions thrown by tasks added with add(), call_at(), call_after() or call_every() are caught and ignored,
     * use call() or call_async() to receive the exception through the returned future.
     */
    void add(venus::function_t function);
    void cancel(venus::call_t::id_t id);

    scheduled_call call_at(const time_point_t & at, function_t function);
    scheduled_call call_after(const duration_t & delay, function_t function);
    scheduled_call call_every(const duration_t & repeat_interval, function_t function);
    scheduled_call call_every(const time_point_t & at, const duration_t & repeat_interval, function_t function);

private:
    bool wait_for_work(const time_point_t timepoint) const;

    /**
     * @brief Executes a single task from the executor's queue.
     *
     * Processes exactly one task or function.
     */
    void run_one();

    /**
     * @brief Stores tasks to be executed as soon as possible, in sequence.
     *
     * The `m_queue` structure manages tasks that should be executed immediately,
     * ensuring strict adherence to the following guarantees:
     * - Tasks in `m_queue` are always executed before any scheduled tasks in `m_scheduled_calls`.
     * - Tasks are executed in the exact order they were added to the queue.
     * - Tasks are executed consecutively (never in parallel), ensuring no race conditions exist between them.
     */
    synchronized_queue<std::function<void()>> m_queue;

    /**
     * @brief Stores tasks along with their scheduled execution times.
     *
     * The `m_scheduled_calls` structure manages tasks that are scheduled to execute
     * at specific times in the future. The primary guarantee is that tasks will not
     * be executed before their scheduled time.
     *
     * Scheduled tasks in `m_scheduled_calls` are processed only after all tasks
     * in the immediate task queue (`m_queue`) have been executed.
     *
     * Tasks can have a `repeat_duration`, indicating that they will be rescheduled
     * after completion at the time point `start time + repeat_duration`.
     *
     * If a task's execution exceeds its `repeat_duration`, the next instance will be scheduled in
     * the past, resulting in immediate execution as soon as the system has capacity.
     * This effectively allows the task to "catch up." (assuming its slow execution was due to temporary lack of resources)
     *
     */
    scheduled_calls m_scheduled_calls;

    /**
     * @brief Source of the ids of scheduled calls, 64 bits so it does not wrap around in practice.
     *
     * Atomic because call_at(), call_after() and call_every() can be called from any thread.
     */
    std::atomic<call_t::id_t> m_last_call_id = {0};

    // scheduled_call handles hold a weak_ptr to this, so cancel() after destruction is detected
    friend class scheduled_call;
    std::shared_ptr<executor *> m_self = std::make_shared<executor *>(this);

    std::atomic<std::thread::id> m_threadId = {};

    bool m_end = false;
    std::thread m_thread;
};

} // namespace venus
