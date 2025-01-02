/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include "executor/executor.hpp"
#include "executor/scheduled_calls.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <utility>

namespace venus {

[[nodiscard]] scheduled_call::id_t make_callid()
{
    static std::atomic<scheduled_call::id_t> id(0);
    return ++id;
}

scheduled_call::scheduled_call(venus::executor & executor, scheduled_call::id_t id) :
    m_executor(&executor),
    m_id(id)
{
}

void scheduled_call::cancel()
{
    if (m_executor == nullptr)
    {
        return;
    }

    m_executor->cancel(m_id);
    m_executor = nullptr;
}

scheduled_call::id_t scheduled_call::id() const
{
    return m_id;
}

// executor

executor::executor() :
    m_thread([this] { run(); })
{
    synchronize();
}

executor::~executor()
{
    add([this] { m_end = true; });
    m_thread.join();
}

bool executor::is_executor_thread() const
{
    return std::this_thread::get_id() == m_threadId;
}

void executor::add(std::function<void()> fn)
{
    m_queue.push(fn);
}

void executor::synchronize()
{
    assert(!is_executor_thread() && "Calling synchronize() inside call() will cause a deadlock");
    std::promise<bool> sync;
    add([&sync]() { sync.set_value(true); });
    sync.get_future().get();
}


scheduled_call executor::call_at(const time_point_t & at, function_t function)
{
    return call_every(at, {}, function);
}

scheduled_call executor::call_after(const duration_t & delay, function_t function)
{
    return call_at(std::chrono::steady_clock::now() + delay, function);
}

scheduled_call executor::call_every(const duration_t & repeat_interval, function_t function)
{
    return call_every(std::chrono::steady_clock::now(), repeat_interval, function);
}

scheduled_call executor::call_every(const time_point_t & at, const duration_t & repeat_interval, function_t function)
{
    auto id = make_callid();
    add([this, id, at, repeat_interval, function]() {
        m_scheduled_calls.insert(venus::call_t(id, at, repeat_interval, function));
    });
    return scheduled_call(*this, id);
}

void executor::cancel(const call_t::id_t id)
{
    if (is_executor_thread())
    {
        m_scheduled_calls.remove(id);
    }
    else
    {
        call([this, id]() { m_scheduled_calls.remove(id); });
    }
}


void executor::run()
{
    m_threadId = std::this_thread::get_id();
    while (!m_end)
    {
        try
        {
            run_one();
        }
        catch (std::exception & e)
        {
            // cdbg << "executor: exception ignored: " << e.what() << "\n";
        }
        catch (...)
        {
            // cdbg << "executor: exception ignored\n";
        }
    }
}

void executor::run_one()
{
    // if there are scheduled_calls and the deadline of first call has expired, execute it.
    if (!m_scheduled_calls.empty() && !wait_for_not_empty(m_scheduled_calls.next_deadline()))
    {
        m_scheduled_calls.pop_front().m_function();
    }
    else
    {
        // either there are no scheduled_calls and pop() will block until there is work to do
        // or there is
        return m_queue.pop()();
    }
}

bool executor::wait_for_not_empty(const time_point_t timepoint) const
{
    return m_queue.wait_for_not_empty(timepoint);
}


} // namespace venus
