/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fmt/core.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <cstdlib> // For setenv
#include <ctime> // For tzset and localtime

#include <fmt/core.h>

#include "support.hpp"

using namespace std::chrono_literals;

enum class Result
{
    Empty,
    Timeout,
    Value
};

// this test checks if changing the realtime clock will effect a condition_value 'wait' timeout.
// where the timeout is a duration unrelated to a specific clock
// this behaviour is not guarenteed by the standard, as far as I'm aware.
TEST(chrono, rtc_behaviour_duration)
{
    ASSERT_TRUE(true);

    reset_rtc();

    std::mutex mutex;
    std::condition_variable condition;

    bool stop = false;
    auto result = Result::Empty;

    auto wait_function = [&] {
        std::unique_lock<std::mutex> lock(mutex);
        auto wait_result = condition.wait_for(lock, 30s, [&]() { return stop; });
        if (wait_result)
        {
            // stop was set
            result = Result::Value;
        }
        else
        {
            result = Result::Timeout;
        }
    };

    std::thread t(wait_function);

    ASSERT_EQ(result, Result::Empty);

    forward_rtc();

    condition.notify_one();
    std::this_thread::sleep_for(2s);

    stop = true;
    condition.notify_one();
    t.join();

    reset_rtc();

    if (result == Result::Timeout)
    {
        log_note("Unexpected", "Changing the RTC caused a timeout!\n");
    }
    else
    {
        log_note("Normal", "Changing the RTC did nothing stop was set to true.\n");
    }
}

// this test checks if changing the timezone will effect a condition_value 'wait' timeout
// where the timeout is a `std::chrono::system_clock::time_point`
// this behaviour is not guarenteed by the standard, as far as I'm aware.
TEST(chrono, rtc_behaviour_duration_system_clock_timepoint)
{
    ASSERT_TRUE(true);

    reset_rtc();

    std::mutex mutex;
    std::condition_variable condition;

    bool stop = false;
    auto timepoint = std::chrono::system_clock::now() + 30s;
    auto result = Result::Empty;

    auto wait_function = [&] {
        std::unique_lock<std::mutex> lock(mutex);
        auto wait_result = condition.wait_until(lock, timepoint, [&]() { return stop; });
        if (wait_result)
        {
            // stop was set
            result = Result::Value;
        }
        else
        {
            result = Result::Timeout;
        }
    };

    std::thread t(wait_function);

    ASSERT_EQ(result, Result::Empty);

    forward_rtc();

    condition.notify_one();
    for (int i = 0; i < 20; ++i)
    {
        std::this_thread::sleep_for(100ms);
        if (result != Result::Empty)
        {
            break;
        }
    }

    stop = true;
    condition.notify_one();
    t.join();

    reset_rtc();

    if (result == Result::Timeout)
    {
        log_note("Normal", "Changing the RTC caused a timeout as expected.\n");
    }
    else
    {
        log_note("Unexpected", "Changing the RTC did nothing stop was set to true.\n");
    }
}

// this test checks if changing the timezone will effect a condition_value 'wait' timeout.
// where the timeout is a `std::chrono::steady_clock::time_point`
// this behaviour is not guarenteed by the standard, as far as I'm aware.
TEST(chrono, rtc_behaviour_duration_steady_clock_timepoint)
{
    ASSERT_TRUE(true);

    reset_rtc();

    std::mutex mutex;
    std::condition_variable condition;

    bool stop = false;
    auto timepoint = std::chrono::steady_clock::now() + 30s;

    std::atomic<Result> result;
    result = Result::Empty;

    auto wait_function = [&] {
        std::unique_lock<std::mutex> lock(mutex);
        auto wait_result = condition.wait_until(lock, timepoint, [&]() { return stop; });
        if (wait_result)
        {
            // stop was set
            result = Result::Value;
        }
        else
        {
            result = Result::Timeout;
        }
    };

    std::thread t(wait_function);

    ASSERT_EQ(result, Result::Empty);

    forward_rtc();

    condition.notify_one();

    for (int i = 0; i < 20; ++i)
    {
        std::this_thread::sleep_for(100ms);
        if (result != Result::Empty)
        {
            break;
        }
    }

    stop = true;
    condition.notify_one();
    t.join();

    reset_rtc();

    if (result == Result::Timeout)
    {
        log_note("Unexpected", "Changing the RTC caused a timeout!\n");
    }
    else
    {
        log_note("Normal", "Changing the RTC did nothing stop was set to true.\n");
    }
}
