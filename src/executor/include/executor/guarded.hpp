/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace venus {

// template <typename T>
// class guarded
// {
// private:
//     T m_data;
//     std::mutex m_mutex;

// public:
//     template <typename Func>
//     auto with_lock(Func && func)
//     {
//         std::lock_guard<std::mutex> lock(m_mutex);
//         return func(m_data);
//     }
// };

template <typename T>
class guarded_notify
{
private:
    T m_data;
    std::mutex m_mutex;
    std::condition_variable m_condition;

public:
    /**
     * @brief Executes the provided @p action while holding the lock.
     *
     * This function acquires a lock on the internal mutex, ensuring thread safety,
     * and then invokes the specified @p action
     *
     * @param action The callable object (e.g., a lambda or function) to be executed.
     * @return The result of invoking @p action
     */
    template <typename Action>
    auto with_lock(Action && action)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return action(m_data);
    }

    /**
     * @brief Waits for the @p condition to be true
     *
     * This function acquires a lock on the internal mutex and waits for the condition
     * variable to be notified.
     *
     * @note This method has no timeout so can possibly block forever
     */
    template <typename Condition>
    void wait_for(Condition && condition)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [&]() { return condition(m_data); });
    }

    /**
     * @brief Waits for the @p condition to be true or a timepoint is reached
     *
     * This function locks the internal mutex and waits for the condition variable to be notified.
     *
     * @return The most recent result of @p condition before returning to the caller.
     *
     * @note The @p condition may be evaluated multiple times due to spurious wakeups.
     */
    template <typename Condition, typename Timepoint>
    auto wait_for(Condition && condition, Timepoint timepoint)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_until(lock, timepoint, [&]() { return condition(m_data); });
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
