// venus::executor: one thread executes all tasks in order, so the data it owns needs no mutex.
#include "executor/executor.hpp"

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

int main()
{
    venus::executor executor;
    int counter = 0; // only accessed by tasks on the executor

    // many threads add work, the executor serializes it
    std::vector<std::thread> producers;
    for (int p = 0; p < 4; ++p)
    {
        producers.emplace_back([&] {
            for (int i = 0; i < 1000; ++i)
            {
                executor.add([&] { ++counter; });
            }
        });
    }
    for (auto & producer : producers)
    {
        producer.join();
    }

    // timers run on the same thread
    auto timer = executor.call_every(std::chrono::milliseconds(1), [&] { ++counter; });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    timer.cancel();

    // call() runs a task and waits for its result
    const int result = executor.call([&] { return counter; });
    std::printf("counter: %d\n", result);
    return result > 4000 ? 0 : 1;
}
