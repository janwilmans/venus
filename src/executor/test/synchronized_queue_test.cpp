/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include "gmock/gmock.h"
#include <atomic>
#include <future>
#include <gtest/gtest.h>

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "executor/synchronized_queue.hpp"

using namespace std::chrono_literals;

TEST(synchronized_queue, construction_destruction)
{
    venus::synchronized_queue<std::vector<std::string>> m_synch_vector;
}

TEST(synchronized_queue, basic_use)
{
    venus::synchronized_queue<std::string> synchronous_string_q;
    ASSERT_EQ(synchronous_string_q.maximum_size(), 0);
    ASSERT_TRUE(synchronous_string_q.empty());
    ASSERT_FALSE(synchronous_string_q.full());
    synchronous_string_q.push("test");
    ASSERT_FALSE(synchronous_string_q.full());
    ASSERT_FALSE(synchronous_string_q.empty());
    ASSERT_EQ(synchronous_string_q.maximum_size(), 0);
}

TEST(synchronized_queue, maximum_size)
{
    venus::synchronized_queue<std::string> synchronous_string_q(1);
    ASSERT_EQ(synchronous_string_q.maximum_size(), 1);
    ASSERT_TRUE(synchronous_string_q.empty());
    ASSERT_FALSE(synchronous_string_q.full());
    synchronous_string_q.push("test");
    ASSERT_TRUE(synchronous_string_q.full());
    ASSERT_FALSE(synchronous_string_q.empty());
    ASSERT_EQ(synchronous_string_q.maximum_size(), 1);
}

TEST(synchronized_queue, queue_order)
{
    venus::synchronized_queue<std::string> synchronous_string_q;
    ASSERT_EQ(synchronous_string_q.size(), 0);
    ASSERT_TRUE(synchronous_string_q.empty());
    synchronous_string_q.push("one");
    synchronous_string_q.push("two");
    synchronous_string_q.push("three");
    ASSERT_FALSE(synchronous_string_q.empty());
    ASSERT_EQ(synchronous_string_q.size(), 3);
    ASSERT_EQ(synchronous_string_q.pop(), "one");
    ASSERT_EQ(synchronous_string_q.size(), 2);
    ASSERT_EQ(synchronous_string_q.pop(), "two");
    ASSERT_EQ(synchronous_string_q.size(), 1);
    ASSERT_EQ(synchronous_string_q.pop(), "three");
    ASSERT_EQ(synchronous_string_q.size(), 0);
    ASSERT_TRUE(synchronous_string_q.empty());
}

TEST(synchronized_queue, move_only_elements)
{
    venus::synchronized_queue<std::unique_ptr<int>> queue;
    queue.push(std::unique_ptr<int>(new int(42)));
    auto value = queue.pop();
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(*value, 42);
}

TEST(synchronized_queue, wait_for_not_full_with_timeout)
{
    venus::synchronized_queue<int> queue(1);
    ASSERT_TRUE(queue.wait_for_not_full(1ms));
    queue.push(1);
    ASSERT_FALSE(queue.wait_for_not_full(1ms));
    ASSERT_EQ(queue.pop(), 1);
    ASSERT_TRUE(queue.wait_for_not_full(1ms));
}

// with one condition variable shared by 'not full' and 'not empty' waiters, notify_one() could wake
// a waiter of the wrong kind, the notification would then be lost and this test would hang.
TEST(synchronized_queue, bounded_multiple_producers_and_consumers)
{
    venus::synchronized_queue<int> queue(1);
    const int items_per_producer = 20000;
    std::atomic<long long> sum(0);

    auto produce = [&] {
        for (int i = 1; i <= items_per_producer; ++i)
        {
            queue.push(i);
        }
    };
    auto consume = [&] {
        for (int i = 0; i < items_per_producer; ++i)
        {
            sum += queue.pop();
        }
    };

    std::vector<std::thread> threads;
    threads.emplace_back(produce);
    threads.emplace_back(produce);
    threads.emplace_back(consume);
    threads.emplace_back(consume);
    for (auto & thread : threads)
    {
        thread.join();
    }

    const long long expected = 2LL * items_per_producer * (items_per_producer + 1) / 2;
    ASSERT_EQ(sum.load(), expected);
    ASSERT_TRUE(queue.empty());
}
