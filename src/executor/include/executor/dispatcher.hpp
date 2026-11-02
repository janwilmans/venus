/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/detail/worker_pool.hpp"
#include "executor/task.hpp"

#include <cstddef>
#include <future>
#include <memory>
#include <utility>

namespace venus {

/**
 * @brief Priority of work in a dispatcher, a worker always picks the oldest task of the highest priority.
 */
enum class priority
{
    high,
    normal,
    low
};

class dispatcher;

/**
 * @brief Executes tasks one at a time, in FIFO order, on the threads of a dispatcher.
 *
 * A serial_queue is not a thread, it is a cheap object: create one per producer, data structure or
 * subsystem that needs its tasks serialized. Many serial queues share the cores of one dispatcher.
 *
 * Guarantees:
 * - tasks of one serial_queue are executed in the order they were dispatched
 * - tasks of one serial_queue never run concurrently, so they can share data without locks
 * - there is no ordering between different serial queues
 *
 * serial_queue is a handle, copies refer to the same queue. Tasks already dispatched are executed,
 * even when all handles are destroyed. The dispatcher must outlive all its serial_queue handles.
 */
class serial_queue
{
public:
    /**
     * @brief Adds a task, exceptions thrown by the task are caught and ignored.
     */
    void dispatch(task t);

    /**
     * @brief Adds a task and returns a future for its result, or the exception it throws.
     */
    template <typename Fn>
    auto dispatch_async(Fn fn)
    {
        std::packaged_task<decltype(fn())()> packaged(std::move(fn));
        auto future = packaged.get_future();
        dispatch(std::move(packaged));
        return future;
    }

    struct state; // implementation detail, defined in dispatcher.cpp

private:
    friend class dispatcher;
    explicit serial_queue(std::shared_ptr<state> shared_state);

    std::shared_ptr<state> m_state;
};

/**
 * @brief Distributes tasks from any number of producers over a fixed number of threads, by priority.
 *
 * Inspired by Grand Central Dispatch, taking the parts that give most of the benefit:
 * - one fixed pool of threads, by default one per core, instead of a thread per producer
 * - priorities: a worker always picks the oldest task of the highest priority (see venus::priority)
 * - concurrent work: dispatch() runs tasks in parallel, like pool_executor
 * - serial work: make_serial_queue() creates cheap queues whose tasks run one at a time, in order
 *
 * A serial queue with work occupies at most one thread at a time. After a batch of tasks it goes to the back of
 * its priority level, so a busy serial queue cannot starve others of the same priority.
 * Higher priority work does always go first, so a continuous stream of high priority work starves lower priorities.
 *
 * The destructor waits until all tasks, including tasks dispatched by tasks, have finished.
 */
class dispatcher
{
public:
    explicit dispatcher(std::size_t thread_count = default_thread_count());

    /**
     * @brief Adds a task to run concurrently with other tasks, exceptions thrown by the task are caught and ignored.
     */
    void dispatch(task t, priority p = priority::normal);

    /**
     * @brief Adds a task to run concurrently with other tasks and returns a future for its result, or the exception it throws.
     */
    template <typename Fn>
    auto dispatch_async(Fn fn, priority p = priority::normal)
    {
        std::packaged_task<decltype(fn())()> packaged(std::move(fn));
        auto future = packaged.get_future();
        dispatch(std::move(packaged), p);
        return future;
    }

    /**
     * @brief Creates a serial queue whose tasks run at priority @p p
     */
    [[nodiscard]] serial_queue make_serial_queue(priority p = priority::normal);

    /**
     * @brief Blocks until all tasks, including those in serial queues, are finished. Must not be called from a task.
     */
    void wait_idle();

    [[nodiscard]] std::size_t thread_count() const;

private:
    detail::worker_pool m_pool;
};

} // namespace venus
