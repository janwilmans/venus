/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include "gmock/gmock.h"
#include <future>
#include <gtest/gtest.h>

#include <fmt/core.h>
#include <memory>
#include <mutex>
#include <string>
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
