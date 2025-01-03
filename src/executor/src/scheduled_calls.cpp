/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include "executor/scheduled_calls.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>

namespace venus {

call_t::call_t(call_t::id_t id, time_point_t at, function_t function) :
    m_id(id),
    m_at(at),
    m_repeat_interval(duration_t::zero()),
    m_function(std::move(function))
{
}

call_t::call_t(call_t::id_t id, time_point_t at, duration_t interval, function_t function) :
    m_id(id),
    m_at(at),
    m_repeat_interval(interval),
    m_function(std::move(function))
{
}

bool scheduled_calls::empty() const
{
    return m_calls.empty();
}

// calls are added in-order, ordered from first to last in time
void scheduled_calls::insert(call_t && call)
{
    auto it = std::lower_bound(m_calls.begin(), m_calls.end(), call, [](const call_t & a, const call_t & b) { return a.m_at > b.m_at; });
    m_calls.insert(it, call);
}

void scheduled_calls::remove(call_t::id_t id)
{
    auto it = std::find_if(m_calls.begin(), m_calls.end(), [id](const call_t & call) { return call.m_id == id; });
    if (it != m_calls.end())
    {
        m_calls.erase(it);
    }
}

time_point_t scheduled_calls::next_deadline() const
{
    assert(!m_calls.empty());
    return m_calls.back().m_at;
}

call_t scheduled_calls::pop_and_reschedule()
{
    call_t call(std::move(m_calls.back()));
    m_calls.pop_back();
    if (call.m_repeat_interval != duration_t::zero())
    {
        insert(call_t(call.m_id, call.m_at + call.m_repeat_interval, call.m_repeat_interval, call.m_function));
    }
    return call;
}

} // namespace venus
