#pragma once

#include "executor/synchronized_queue.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace venus {

class executor;

using callid_t = std::uint32_t;

class scheduled_call
{
public:
    scheduled_call();
    void cancel();

private:
    scheduled_call(venus::executor & executor, callid_t id);
    [[nodiscard]] callid_t id() const;

    executor * m_executor;
    callid_t m_id;
};

class executor
{
public:
    template <typename Fn>
    auto call(Fn fn)
    {
        if (is_executor_thread())
        {
            assert(true && "calling call inside the executor thread is usually a mistake");
            return fn();
        }
        std::packaged_task<decltype(fn())()> task(fn);
        Add([&task]() { task(); });
        return task.get_future().get();
    }

    template <typename Fn>
    auto call_async(Fn fn)
    {
        auto pTask = std::make_shared<std::packaged_task<decltype(fn())()>>(fn);
        auto f = pTask->get_future();
        Add([pTask]() { (*pTask)(); });
        return f;
    }

    bool is_executor_thread() const;

    void run_one();
    void synchronize();

protected:
    void synchronize_internally();
    void set_executor_thread();
    void set_executor_thread(std::thread::id id);

    void add(std::function<void()> function);

    // bool wait_for_not_empty(const std::chrono::nanoseconds time) const
    // {
    //     return m_queue.wait_for_not_empty(time);
    // }

private:
    synchronized_queue<std::function<void()>> m_queue;
    std::atomic<std::thread::id> m_threadId;

    bool m_end;
    std::thread m_thread;
};

} // namespace venus