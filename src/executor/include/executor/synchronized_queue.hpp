#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace venus {

template <typename T>
class synchronized_queue
{
public:
    explicit synchronized_queue(size_t maximum_size = 0) :
        m_maximum_size(maximum_size)
    {
    }

    [[nodiscard]] bool empty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return empty(lock);
    }

    [[nodiscard]] bool full() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return full(lock);
    }

    [[nodiscard]] size_t size() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_q.size();
    }

    [[nodiscard]] size_t maximum_size() const
    {
        return m_maximum_size;
    }

    void wait_for_not_full() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return !full(lock); });
    }

    // even through a relative time is given, a change to the system_clock can trigger a spurious wake-up ?
    // test...
    bool wait_for_not_full(std::chrono::nanoseconds duration) const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for(lock, duration, [&]() { return !full(lock); });
    }

    void wait_for_not_empty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return !empty(lock); });
    }

    // even through a relative time is given, a change to the system_clock can trigger a spurious wake-up ?
    // test...
    bool wait_for_not_empty(std::chrono::nanoseconds duration) const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for(lock, duration, [&]() { return !empty(lock); });
    }

    void push(T t)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return !full(lock); });
        m_q.push(std::move(t));
        lock.unlock();
        m_condition.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return !empty(lock); });
        T t(m_q.front());
        m_q.pop();
        lock.unlock();
        m_condition.notify_one();
        return t;
    }

private:
    [[nodiscard]] bool empty(std::unique_lock<std::mutex> &) const
    {
        return m_q.empty();
    }

    [[nodiscard]] bool full(std::unique_lock<std::mutex> &) const
    {
        return m_maximum_size > 0 && m_q.size() == m_maximum_size;
    }

    size_t m_maximum_size;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    std::queue<T> m_q;
};

} // namespace venus