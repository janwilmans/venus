/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace venus {

/**
 * @brief Holds a T that is only accessible while holding a lock, and a condition variable to wait for changes to it.
 *
 * Different waiters may wait for different conditions on the same condition variable,
 * this is why every change notifies _all_ waiters, with notify_one() a waiter for the 'wrong' condition
 * could consume the notification, while a waiter for the 'right' condition keeps waiting (lost wake-up).
 */
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
     * @brief Waits for the @p condition to be true or the @p duration to pass
     *
     * @return The most recent result of @p condition before returning to the caller.
     */
    template <typename Condition, typename Rep, typename Period>
    bool wait_for(Condition && condition, const std::chrono::duration<Rep, Period> & duration)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for(lock, duration, [&]() { return condition(m_data); });
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
    template <typename Condition, typename Clock, typename Duration>
    bool wait_until(Condition && condition, const std::chrono::time_point<Clock, Duration> & timepoint)
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
        m_condition.notify_all();
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
        m_condition.notify_all();
        return result;
    }
};

} // namespace venus
