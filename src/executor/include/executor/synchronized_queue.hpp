/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/guarded.hpp"

#include <chrono>
#include <cstddef>
#include <queue>
#include <utility>

/*
 * Implementation note on waiting until a time_point:
 * https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until
 *
 * The standard recommends that the clock tied to the time_point is used to measure time.
 * libstdc++ before GCC 10 does not: it converts a steady_clock time_point to system_clock and waits on that,
 * so adjusting the system clock can make a wait end early or late.
 * GCC 10+ (pthread_cond_clockwait) and MSVC wait on the steady clock as intended.
 *
 * In any case, the function also may wait for longer than until after the time_point has been reached
 * due to scheduling or resource contention delays.
 */

namespace venus {

template <typename T>
class synchronized_queue
{
private:
    std::size_t m_maximum_size; // no synchronization needed, set only at construction

    using TQueue = std::queue<T>;
    mutable guarded_notify<TQueue> m_queue;

public:
    explicit synchronized_queue(std::size_t maximum_size = 0) :
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

    [[nodiscard]] std::size_t size() const
    {
        return m_queue.with_lock([](const TQueue & queue) {
            return queue.size();
        });
    }

    [[nodiscard]] std::size_t maximum_size() const
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

    /**
     * @brief Waits for the queue to have room for at least one element, or the @p duration to pass
     *
     * @return `true` if the queue is not full; `false` if the duration passed.
     */
    template <typename Rep, typename Period>
    [[nodiscard]] bool wait_for_not_full(const std::chrono::duration<Rep, Period> & duration) const
    {
        if (m_maximum_size == 0)
        {
            return true;
        }

        return m_queue.wait_for([this](const TQueue & queue) { return queue.size() < m_maximum_size; }, duration);
    }

    void wait_for_not_empty() const
    {
        m_queue.wait_for([](const TQueue & queue) { return !queue.empty(); });
    }

    /**
     * @brief Waits for the queue to become non-empty or a timepoint is reached
     *
     * This function blocks the calling thread until either the queue is no longer empty
     * or the specified timepoint is reached, whichever occurs first.
     *
     * @param timepoint The timepoint to wait until for the queue to become non-empty.
     *
     * @return `true` if the queue became non-empty before the timepoint was reached;
     *         `false` otherwise.
     */
    template <typename Clock, typename Duration>
    [[nodiscard]] bool wait_for_not_empty(const std::chrono::time_point<Clock, Duration> & timepoint) const
    {
        return m_queue.wait_until([](const TQueue & queue) { return !queue.empty(); }, timepoint);
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
            [](const TQueue & queue) { return !queue.empty(); },
            [](TQueue & queue) { T result = std::move(queue.front()); queue.pop(); return result; });
    }
};

} // namespace venus
