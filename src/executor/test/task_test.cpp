/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include <gtest/gtest.h>

#include <future>
#include <memory>
#include <utility>

#include "executor/task.hpp"

TEST(task, default_constructed_is_empty)
{
    venus::task task;
    ASSERT_FALSE(task);
}

TEST(task, calls_the_function)
{
    int count = 0;
    venus::task task([&] { ++count; });
    ASSERT_TRUE(task);
    task();
    task();
    ASSERT_EQ(count, 2);
}

TEST(task, accepts_move_only_callables)
{
    auto value = std::make_unique<int>(41);
    int result = 0;
    venus::task task([value = std::move(value), &result]() { result = *value + 1; });
    task();
    ASSERT_EQ(result, 42);
}

TEST(task, accepts_packaged_task)
{
    std::packaged_task<int()> packaged([] { return 42; });
    auto future = packaged.get_future();
    venus::task task(std::move(packaged));
    task();
    ASSERT_EQ(future.get(), 42);
}

TEST(task, move_transfers_the_callable)
{
    int count = 0;
    venus::task first([&] { ++count; });
    venus::task second(std::move(first));
    ASSERT_FALSE(first); // NOLINT(bugprone-use-after-move): moved-from state is specified
    ASSERT_TRUE(second);
    second();

    venus::task third;
    third = std::move(second);
    ASSERT_FALSE(second); // NOLINT(bugprone-use-after-move)
    third();
    ASSERT_EQ(count, 2);
}

TEST(task, accepts_lvalue_callables)
{
    int count = 0;
    auto increment = [&] { ++count; };
    venus::task task(increment);
    task();
    increment();
    ASSERT_EQ(count, 2);
}
