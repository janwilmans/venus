#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace venus {

template <typename T>
class naive_guarded
{
private:
    T data;
    mutable std::mutex mtx;

public:
    template <typename Func>
    auto with_lock(Func && func)
    {
        std::lock_guard<std::mutex> lock(mtx);
        return func(data);
    }
};

} // namespace venus