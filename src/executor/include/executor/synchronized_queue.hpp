#pragma once

#include "executor/guarded.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

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

    [[nodiscard]] bool wait_for_not_empty(std::chrono::nanoseconds duration) const
    {
        return m_queue.wait_for([this](const TQueue & queue) { return !queue.empty(); }, duration);
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