/*
 * Copyright (c) 2025 Jan Wilmans
 */

// checks that the generated single header is complete and self-contained

#include "venus.hpp"

#include <cstdio>

int main()
{
    venus::executor executor;
    venus::pool_executor pool(2);
    venus::dispatcher dispatcher(2);
    auto queue = dispatcher.make_serial_queue(venus::priority::high);
    venus::synchronized_queue<int> synchronized(1);
    synchronized.push(4);

    const int a = executor.call([] { return 1; });
    const int b = pool.call_async([] { return 2; }).get();
    const int c = queue.dispatch_async([] { return 3; }).get();
    const int d = synchronized.pop();

    std::printf("single header: %d\n", a + b + c + d);
    return a + b + c + d == 10 ? 0 : 1;
}
