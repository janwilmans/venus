#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace venus {

template <typename T>
class guarded
{
private:
    T m_data;
    std::mutex m_mutex;

public:
    template <typename Func>
    auto with_lock(Func && func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return func(m_data);
    }
};

template <typename T>
class guarded_notify
{
private:
    T m_data;
    std::mutex m_mutex;
    std::condition_variable m_condition;

public:
    /**
     * @brief executes @p action after waiting for @p condition, where @p action returns a result
     */
    template <typename Action>
    auto with_lock(Action && action)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return action(m_data);
    }

    /**
     * @brief wait for the @p condition to be true
     */
    template <typename Condition>
    void wait_for(Condition && condition)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return condition(m_data); });
    }

    /**
     * @brief wait for the @p condition to be true or a timeout occurs
     * @return the latest result of condition() before returning to the caller.
     */
    template <typename Condition, typename Duration>
    auto wait_for(Condition && condition, Duration duration)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait(lock, duration, [&]() { return condition(m_data); });
    }

    /**
     * @brief executes @p action after waiting for @p condition
     */
    template <typename Condition, typename Action>
    void with_lock_and_notify(Condition && condition, Action && action)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return condition(m_data); });

        action(m_data);
        lock.unlock();
        m_condition.notify_one();
    }

    /**
     * @brief executes @p action after waiting for @p condition, where @p action returns a result
     * @return the result of action()
     */
    template <typename Condition, typename Action>
    auto with_lock_and_notify_r(Condition && condition, Action && action)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return condition(m_data); });

        auto result = action(m_data);
        lock.unlock();
        m_condition.notify_one();
        return result;
    }
};

} // namespace venus
