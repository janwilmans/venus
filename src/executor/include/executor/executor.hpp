/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/scheduled_calls.hpp"
#include "executor/synchronized_queue.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace venus {

class executor;
class scheduled_call
{
public:
    using id_t = std::uint32_t;

    scheduled_call(venus::executor & executor, scheduled_call::id_t id);
    void cancel();

    [[nodiscard]] scheduled_call::id_t id() const;

private:
    executor * m_executor;
    scheduled_call::id_t m_id;
};

[[nodiscard]] scheduled_call::id_t make_callid();

class executor
{
public:
    executor();

    /**
     * @brief The destructor of the executor ensures that any ongoing tasks initiated by call(), tasks scheduled via call_async, or
     * call_at / call_after that have reached their scheduled time, are completed before the destructor finishes execution.
     *
     * Note: users still using the executor during destruction are in violation of the C++ object lifetime rules.
     */
    ~executor();

    template <typename Fn>
    auto call(Fn fn)
    {
        if (is_executor_thread())
        {
            assert(false && "calling call() inside the executor thread is usually a mistake");
            return fn();
        }

        // A packaged_task encapsulates a task and its associated promise in one object.
        std::packaged_task<decltype(fn())()> task(fn);
        add([&task]() { task(); });
        return task.get_future().get();
    }

    template <typename Fn>
    auto call_async(Fn fn)
    {
        auto pTask = std::make_shared<std::packaged_task<decltype(fn())()>>(fn);
        auto f = pTask->get_future();
        add([pTask]() { (*pTask)(); });
        return f;
    }

    /**
     * @brief Checks if the calling thread is the executor's designated thread.
     *
     * @return `true` if the calling thread is the executor thread; otherwise, `false`.
     */
    bool is_executor_thread() const;

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
     */
    void synchronize();

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

    std::atomic<std::thread::id> m_threadId = {};

    bool m_end = false;
    std::thread m_thread;
};

} // namespace venus
