/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "executor/dispatcher.hpp"

using namespace std::chrono_literals;

namespace {

// occupies a worker thread until open() is called
class gate
{
public:
    explicit gate(venus::dispatcher & dispatcher)
    {
        std::promise<void> started;
        auto started_future = started.get_future();
        auto opened = m_open.get_future().share();
        dispatcher.dispatch(
            [opened, started = std::move(started)]() mutable {
                started.set_value();
                opened.wait();
            },
            venus::priority::high);
        started_future.wait();
    }

    void open()
    {
        m_open.set_value();
    }

private:
    std::promise<void> m_open;
};

} // namespace

TEST(dispatcher, runs_all_tasks)
{
    std::atomic<int> count(0);
    {
        venus::dispatcher dispatcher(4);
        for (int i = 0; i < 10000; ++i)
        {
            dispatcher.dispatch([&] { ++count; });
        }
    }
    ASSERT_EQ(count.load(), 10000);
}

TEST(dispatcher, higher_priority_runs_first)
{
    venus::dispatcher dispatcher(1);
    std::string order;

    gate blocked(dispatcher);
    dispatcher.dispatch([&] { order += 'L'; }, venus::priority::low);
    dispatcher.dispatch([&] { order += 'N'; }, venus::priority::normal);
    dispatcher.dispatch([&] { order += 'H'; }, venus::priority::high);
    blocked.open();
    dispatcher.wait_idle();

    ASSERT_EQ(order, "HNL");
}

TEST(dispatcher, dispatch_async_returns_result_and_exception)
{
    venus::dispatcher dispatcher(2);
    auto result = dispatcher.dispatch_async([] { return 42; }, venus::priority::low);
    auto failure = dispatcher.dispatch_async([]() -> int { throw std::runtime_error("expected"); });
    ASSERT_EQ(result.get(), 42);
    ASSERT_THROW(failure.get(), std::runtime_error);
}

// many producers share one serial queue: per producer the order is kept and tasks never overlap
TEST(dispatcher, serial_queue_is_fifo_and_never_concurrent)
{
    const int producers = 4;
    const int tasks_per_producer = 10000;

    venus::dispatcher dispatcher(4);
    auto queue = dispatcher.make_serial_queue();

    std::atomic<int> in_flight(0);
    std::atomic<int> max_in_flight(0);
    std::vector<std::vector<int>> received(producers); // only accessed by tasks of the serial queue

    std::vector<std::thread> threads;
    for (int p = 0; p < producers; ++p)
    {
        threads.emplace_back([&, p] {
            for (int i = 0; i < tasks_per_producer; ++i)
            {
                queue.dispatch([&, p, i] {
                    const int now = ++in_flight;
                    int max = max_in_flight.load();
                    while (now > max && !max_in_flight.compare_exchange_weak(max, now))
                    {
                    }
                    received[static_cast<std::size_t>(p)].push_back(i);
                    --in_flight;
                });
            }
        });
    }
    for (auto & thread : threads)
    {
        thread.join();
    }
    dispatcher.wait_idle();

    ASSERT_EQ(max_in_flight.load(), 1);
    for (const auto & sequence : received)
    {
        ASSERT_EQ(sequence.size(), static_cast<std::size_t>(tasks_per_producer));
        ASSERT_TRUE(std::is_sorted(sequence.begin(), sequence.end()));
    }
}

// every serial queue waits until all have started, this only completes if serial queues run in parallel
TEST(dispatcher, serial_queues_run_in_parallel)
{
    const int queues = 4;
    venus::dispatcher dispatcher(queues);

    std::mutex mutex;
    std::condition_variable all_started;
    int started = 0;
    std::atomic<int> completed(0);

    std::vector<venus::serial_queue> serial_queues;
    for (int i = 0; i < queues; ++i)
    {
        serial_queues.push_back(dispatcher.make_serial_queue());
    }
    for (auto & queue : serial_queues)
    {
        queue.dispatch([&] {
            std::unique_lock<std::mutex> lock(mutex);
            ++started;
            all_started.notify_all();
            if (all_started.wait_for(lock, 10s, [&] { return started == queues; }))
            {
                ++completed;
            }
        });
    }
    dispatcher.wait_idle();
    ASSERT_EQ(completed.load(), queues);
}

// a busy serial queue yields its thread after a batch, so another queue of the same priority is not starved
TEST(dispatcher, busy_serial_queue_does_not_starve_others)
{
    venus::dispatcher dispatcher(1);
    auto busy = dispatcher.make_serial_queue();
    auto other = dispatcher.make_serial_queue();

    int busy_done = 0;
    int busy_done_when_other_ran = -1;

    gate blocked(dispatcher);
    for (int i = 0; i < 1000; ++i)
    {
        busy.dispatch([&] { ++busy_done; });
    }
    other.dispatch([&] { busy_done_when_other_ran = busy_done; });
    blocked.open();
    dispatcher.wait_idle();

    ASSERT_EQ(busy_done, 1000);
    ASSERT_GE(busy_done_when_other_ran, 0);
    ASSERT_LE(busy_done_when_other_ran, 16);
}

TEST(dispatcher, serial_queue_dispatch_async_and_exceptions)
{
    venus::dispatcher dispatcher(2);
    auto queue = dispatcher.make_serial_queue(venus::priority::high);
    queue.dispatch([] { throw std::runtime_error("ignored"); });
    auto failure = queue.dispatch_async([]() -> int { throw std::runtime_error("expected"); });
    auto result = queue.dispatch_async([] { return 42; });
    ASSERT_THROW(failure.get(), std::runtime_error);
    ASSERT_EQ(result.get(), 42);
}

TEST(dispatcher, tasks_run_after_serial_queue_handle_is_destroyed)
{
    venus::dispatcher dispatcher(2);
    std::atomic<int> count(0);
    {
        auto queue = dispatcher.make_serial_queue(venus::priority::low);
        for (int i = 0; i < 10; ++i)
        {
            queue.dispatch([&] {
                std::this_thread::sleep_for(100us);
                ++count;
            });
        }
    }
    dispatcher.wait_idle();
    ASSERT_EQ(count.load(), 10);
}

TEST(dispatcher, destructor_waits_for_serial_and_nested_work)
{
    std::atomic<int> count(0);
    {
        venus::dispatcher dispatcher(4);
        auto queue = dispatcher.make_serial_queue();
        for (int i = 0; i < 100; ++i)
        {
            queue.dispatch([&] {
                dispatcher.dispatch([&] { ++count; }, venus::priority::low);
                ++count;
            });
        }
    }
    ASSERT_EQ(count.load(), 200);
}
