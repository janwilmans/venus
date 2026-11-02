/*
 * Copyright (c) 2025 Jan Wilmans
 */

// The library is header-only: this second translation unit that includes every header checks that
// all functions are inline (otherwise linking fails with duplicate symbols).

#include "executor/dispatcher.hpp"
#include "executor/executor.hpp"
#include "executor/guarded.hpp"
#include "executor/pool_executor.hpp"
#include "executor/scheduled_calls.hpp"
#include "executor/synchronized_queue.hpp"
#include "executor/task.hpp"

#include <gtest/gtest.h>

TEST(headers, usable_from_multiple_translation_units)
{
    venus::executor executor;
    venus::pool_executor pool(1);
    venus::dispatcher dispatcher(1);
    auto queue = dispatcher.make_serial_queue();

    ASSERT_EQ(executor.call([] { return 1; }), 1);
    ASSERT_EQ(pool.call_async([] { return 2; }).get(), 2);
    ASSERT_EQ(queue.dispatch_async([] { return 3; }).get(), 3);
}
