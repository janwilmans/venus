/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include "gmock/gmock.h"
#include <atomic>
#include <future>
#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "executor/executor.hpp"

using namespace std::chrono_literals;

// test that adding immediate tasks are executed before added scheduled tasks
// even if they are scheduled while waiting for the next scheduled task deadline
TEST(executor, ordering)
{
    venus::executor executor;

    enum class mark
    {
        unset,
        scheduled_call,
        immediate
    };

    mark task_mark = mark::unset;

    auto scheduled_call = executor.call_after(5min, [&] { task_mark = mark::scheduled_call; });
    executor.call([&] { task_mark = mark::immediate; });
    scheduled_call.cancel();
    ASSERT_EQ(task_mark, mark::immediate);
}

TEST(executor, sync_async_ordering)
{
    std::vector<int> sequence;
    auto add = [&](int value) {
        sequence.push_back(value);
    };

    {
        venus::executor executor;
        executor.call([&]() { add(1); });
        executor.call_async([&]() { add(2); });
        executor.call_async([&]() { add(3); });
        executor.call([&]() { add(4); });
    }

    ASSERT_THAT(sequence, testing::ElementsAre(1, 2, 3, 4));
}

TEST(executor, ordering_sequence)
{
    venus::executor executor;

    std::mutex mutex;
    std::vector<int> sequence;
    auto add = [&](int value) {
        std::lock_guard<std::mutex> lock(mutex);
        sequence.push_back(value);
    };

    executor.call_after(1001us, [&]() { add(1); });
    executor.call_after(1002us, [&]() { add(2); });
    executor.synchronize();
    executor.call_after(1003us, [&]() { add(3); });
    executor.call_after(1004us, [&]() { add(4); });

    using task_t = std::packaged_task<void()>;
    auto task = std::make_shared<task_t>([&]() {
        add(5);
    });

    auto future = task->get_future();
    executor.call_after(1005us, [task] { (*task)(); });
    future.wait();

    ASSERT_THAT(sequence, testing::ElementsAre(1, 2, 3, 4, 5));
}

// call_every() used to defer inserting the call, while cancel() on the executor thread removes it immediately,
// so a cancel() directly after call_every() inside a task was lost and the call kept repeating forever.
TEST(executor, cancel_call_every_on_executor_thread)
{
    venus::executor executor;
    std::atomic<int> count(0);

    executor.call([&] {
        auto call = executor.call_every(1ms, [&] { ++count; });
        call.cancel();
    });

    std::this_thread::sleep_for(20ms);
    executor.synchronize();
    ASSERT_EQ(count.load(), 0);
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(executor, synchronize_on_executor_thread_throws_instead_of_deadlock)
{
    venus::executor executor;
    ASSERT_THROW(executor.call([&] { executor.synchronize(); }), std::logic_error);
}

TEST(executor, cancel_after_executor_is_destroyed_does_nothing)
{
    auto executor = std::make_unique<venus::executor>();
    auto call = executor->call_after(1h, [] {});
    executor.reset();
    call.cancel();
}
