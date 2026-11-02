/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include "executor/dispatcher.hpp"
#include "executor/executor.hpp"
#include "executor/pool_executor.hpp"

int main()
{
    venus::executor executor;
    venus::pool_executor pool;
    venus::dispatcher dispatcher;
    auto queue = dispatcher.make_serial_queue();

    const int a = executor.call([] { return 1; });
    const int b = pool.call_async([] { return 2; }).get();
    const int c = queue.dispatch_async([] { return 3; }).get();
    return a + b + c == 6 ? 0 : 1;
}
