/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "executor/pool_executor.hpp"

using namespace std::chrono_literals;

TEST(pool_executor, runs_all_tasks)
{
    std::atomic<int> count(0);
    {
        venus::pool_executor pool(4);
        ASSERT_EQ(pool.thread_count(), 4u);
        for (int i = 0; i < 10000; ++i)
        {
            pool.add([&] { ++count; });
        }
    }
    ASSERT_EQ(count.load(), 10000);
}

TEST(pool_executor, zero_threads_means_one)
{
    venus::pool_executor pool(0);
    ASSERT_EQ(pool.thread_count(), 1u);
    ASSERT_EQ(pool.call_async([] { return 1; }).get(), 1);
}

// every task waits until all tasks have started, this only completes if the tasks run in parallel
TEST(pool_executor, runs_tasks_in_parallel)
{
    const int threads = 4;
    venus::pool_executor pool(threads);

    std::mutex mutex;
    std::condition_variable all_started;
    int started = 0;
    std::atomic<int> completed(0);

    for (int i = 0; i < threads; ++i)
    {
        pool.add([&] {
            std::unique_lock<std::mutex> lock(mutex);
            ++started;
            all_started.notify_all();
            if (all_started.wait_for(lock, 10s, [&] { return started == threads; }))
            {
                ++completed;
            }
        });
    }
    pool.wait_idle();
    ASSERT_EQ(completed.load(), threads);
}

TEST(pool_executor, call_async_returns_result)
{
    venus::pool_executor pool(2);
    auto future = pool.call_async([] { return 6 * 7; });
    ASSERT_EQ(future.get(), 42);
}

TEST(pool_executor, call_async_propagates_exception)
{
    venus::pool_executor pool(2);
    auto future = pool.call_async([]() -> int { throw std::runtime_error("expected"); });
    ASSERT_THROW(future.get(), std::runtime_error);
}

TEST(pool_executor, exception_in_task_does_not_stop_the_pool)
{
    venus::pool_executor pool(1);
    pool.add([] { throw std::runtime_error("ignored"); });
    ASSERT_EQ(pool.call_async([] { return 1; }).get(), 1);
}

TEST(pool_executor, accepts_move_only_tasks)
{
    venus::pool_executor pool(2);
    auto value = std::make_unique<int>(42);
    auto future = pool.call_async([value = std::move(value)] { return *value; });
    ASSERT_EQ(future.get(), 42);
}

TEST(pool_executor, destructor_waits_for_tasks_added_by_tasks)
{
    std::atomic<int> count(0);
    {
        venus::pool_executor pool(4);
        for (int i = 0; i < 100; ++i)
        {
            pool.add([&] {
                std::this_thread::sleep_for(100us);
                pool.add([&] { ++count; });
            });
        }
    }
    ASSERT_EQ(count.load(), 100);
}

TEST(pool_executor, wait_idle_waits_for_running_tasks)
{
    venus::pool_executor pool(2);
    std::atomic<int> count(0);
    for (int i = 0; i < 8; ++i)
    {
        pool.add([&] {
            std::this_thread::sleep_for(2ms);
            ++count;
        });
    }
    pool.wait_idle();
    ASSERT_EQ(count.load(), 8);
}

TEST(pool_executor, multiple_producers)
{
    const int producers = 4;
    const int tasks_per_producer = 25000;
    std::atomic<int> count(0);
    {
        venus::pool_executor pool;
        std::vector<std::thread> threads;
        for (int p = 0; p < producers; ++p)
        {
            threads.emplace_back([&] {
                for (int i = 0; i < tasks_per_producer; ++i)
                {
                    pool.add([&] { ++count; });
                }
            });
        }
        for (auto & thread : threads)
        {
            thread.join();
        }
    }
    ASSERT_EQ(count.load(), producers * tasks_per_producer);
}

TEST(pool_executor, wait_idle_from_a_task_throws_instead_of_deadlock)
{
    venus::pool_executor pool(1);
    auto future = pool.call_async([&] { pool.wait_idle(); });
    ASSERT_THROW(future.get(), std::logic_error);
}
