/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/guarded.hpp"
#include "executor/scheduled_calls.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

/*
 * bool wait_until( std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& abs_time, Predicate pred );
 *
 * Implementation note:
 * https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until
 *
 * The standard recommends that the clock tied to abs_time be used to measure time;
 * that clock is NOT required to be a monotonic clock. There are no guarantees regarding the behavior of this
 * function if the clock is adjusted discontinuously, but the existing implementations convert
 * abs_time from Clock to std::chrono::system_clock and delegate to POSIX pthread_cond_timedwait
 * so that the wait honors adjustments to the system clock, but not to the user-provided Clock.
 *
 * In any case, the function also may wait for longer than until after abs_time has been reached
 * due to scheduling or resource contention delays.
 *
 * NOTE: If a task is scheduled for a specific future time, such as 3 AM, and the clock is adjusted (e.g., at 1 AM, the clock is set to 2 AM),
 * the task will still execute at 3 AM based on the adjusted clock, even though this occurs an hour earlier than originally anticipated.
 * TODO: verify this!!!
 */

namespace venus {

template <typename T>
class synchronized_queue
{
private:
    size_t m_maximum_size; // no synchronization needed, set only at construction

    using TQueue = std::queue<T>;
    mutable guarded_notify<TQueue> m_queue;

public:
    explicit synchronized_queue(size_t maximum_size = 0) :
        m_maximum_size(maximum_size)
    {
    }

    [[nodiscard]] bool empty() const
    {
        return m_queue.with_lock([](TQueue & queue) {
            return queue.empty();
        });
    }

    [[nodiscard]] bool full() const
    {
        if (m_maximum_size == 0)
        {
            return false;
        }

        return m_queue.with_lock([this](const TQueue & queue) {
            return queue.size() == m_maximum_size;
        });
    }

    [[nodiscard]] size_t size() const
    {
        return m_queue.with_lock([this](const TQueue & queue) {
            return queue.size();
        });
    }

    [[nodiscard]] size_t maximum_size() const
    {
        return m_maximum_size;
    }

    void wait_for_not_full() const
    {
        if (m_maximum_size == 0)
        {
            return;
        }

        m_queue.wait_for([this](const TQueue & queue) {
            return queue.size() < m_maximum_size;
        });
    }

    // even through a relative time is given, a change to the system_clock can trigger a spurious wake-up ?
    // test...
    [[nodiscard]] bool wait_for_not_full(std::chrono::nanoseconds duration) const
    {
        if (m_maximum_size == 0)
        {
            return true;
        }

        return m_queue.wait_for([this](const TQueue & queue) { return queue.size() < m_maximum_size; }, duration);
    }

    void wait_for_not_empty() const
    {
        m_queue.wait_for([this](const TQueue & queue) { return !queue.empty(); });
    }


    /**
     * @brief Waits for the queue to become non-empty or a timepoint is reached
     *
     * This function blocks the calling thread until either the queue is no longer empty
     * or the specified timepoint is reached, whichever occurs first.
     *
     * @param timeout The timepoint to wait until for the queue to become non-empty.
     *
     * @return `true` if the queue became non-empty before the timepoint was reached;
     *         `false` otherwise.
     */
    [[nodiscard]] bool wait_for_not_empty(const time_point_t timepoint) const
    {
        return m_queue.wait_for([this](const TQueue & queue) { return !queue.empty(); }, timepoint);
    }

    void push(T t)
    {
        m_queue.with_lock_and_notify(
            [this](const TQueue & queue) { return m_maximum_size == 0 || queue.size() != m_maximum_size; },
            [&](TQueue & queue) { queue.push(std::move(t)); });
    }

    T pop()
    {
        return m_queue.with_lock_and_notify_r(
            [this](const TQueue & queue) { return queue.size() > 0; },
            [&](TQueue & queue) { auto result = queue.front(); queue.pop(); return result; });
    }
};

} // namespace venus
