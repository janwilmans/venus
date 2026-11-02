// venus::pool_executor: independent tasks run in parallel on all cores.
#include "executor/pool_executor.hpp"

#include <cstdio>
#include <future>
#include <numeric>
#include <vector>

int main()
{
    std::vector<long long> numbers(1000000);
    std::iota(numbers.begin(), numbers.end(), 1);

    venus::pool_executor pool; // one thread per core
    const std::size_t chunks = 8;
    const std::size_t chunk_size = numbers.size() / chunks;

    // scatter: every chunk is summed by a task, the futures gather the results
    std::vector<std::future<long long>> sums;
    for (std::size_t c = 0; c < chunks; ++c)
    {
        auto first = numbers.begin() + static_cast<std::ptrdiff_t>(c * chunk_size);
        auto last = first + static_cast<std::ptrdiff_t>(chunk_size);
        sums.push_back(pool.call_async([first, last] { return std::accumulate(first, last, 0LL); }));
    }

    long long total = 0;
    for (auto & sum : sums)
    {
        total += sum.get();
    }
    std::printf("threads: %zu, total: %lld\n", pool.thread_count(), total);
    return total == 500000500000LL ? 0 : 1;
}
