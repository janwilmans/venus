// venus::dispatcher: parallel work at different priorities, results applied on a serial queue without locks.
#include "executor/dispatcher.hpp"

#include <cstdio>
#include <map>
#include <string>

int main()
{
    venus::dispatcher dispatcher; // one thread per core, shared by all queues
    auto model_queue = dispatcher.make_serial_queue(venus::priority::high);
    std::map<int, long long> model; // only accessed by tasks on model_queue

    for (int n = 1; n <= 20; ++n)
    {
        // heavy work runs in parallel, at low priority
        dispatcher.dispatch([&, n] {
            long long sum = 0;
            for (int i = 1; i <= n * 100000; ++i)
            {
                sum += i % 7;
            }
            // the result is applied on the serial queue: one task at a time, in order, no mutex needed
            model_queue.dispatch([&, n, sum] { model[n] = sum; });
        }, venus::priority::low);
    }

    dispatcher.wait_idle();
    const auto size = model_queue.dispatch_async([&] { return model.size(); }).get();
    std::printf("results: %zu, model[20]: %lld\n", size, model[20]);
    return size == 20 ? 0 : 1;
}
