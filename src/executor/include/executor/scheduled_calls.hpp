/*
 * Copyright (c) 2025 Jan Wilmans
 */

#pragma once

#include "executor/detail/config.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace venus {

using clock_type = std::chrono::steady_clock; // not clock_t, that would clash with ::clock_t from <ctime>
using time_point_t = clock_type::time_point;
using duration_t = clock_type::duration;
using function_t = std::function<void()>;

struct call_t
{
    using id_t = std::uint64_t;
    call_t(call_t::id_t id, time_point_t at, function_t function);
    call_t(call_t::id_t id, time_point_t at, duration_t repeat_interval, function_t function);

    call_t::id_t m_id;
    time_point_t m_at;
    duration_t m_repeat_interval;
    function_t m_function;
};

class scheduled_calls
{
public:
    VENUS_NODISCARD bool empty() const;
    void insert(call_t && call);
    void remove(call_t::id_t);
    VENUS_NODISCARD time_point_t next_deadline() const;

    /**
     * @brief Pops the first task from `m_calls` and potentially reschedules it.
     *
     * This function gets the first task from `m_calls`, and determines
     * whether it should be rescheduled based on its `m_repeat_interval` property.
     * If the task has a repeat duration, it will be reinserted into the list at an
     * appropriate position based on its next execution time.
     *
     * @return the first task taken from `m_calls`
     */
    call_t pop_and_reschedule();

private:
    std::vector<call_t> m_calls;
};

// implementation

inline call_t::call_t(call_t::id_t id, time_point_t at, function_t function) :
    m_id(id),
    m_at(at),
    m_repeat_interval(duration_t::zero()),
    m_function(std::move(function))
{
}

inline call_t::call_t(call_t::id_t id, time_point_t at, duration_t interval, function_t function) :
    m_id(id),
    m_at(at),
    m_repeat_interval(interval),
    m_function(std::move(function))
{
}

inline bool scheduled_calls::empty() const
{
    return m_calls.empty();
}

// calls are added in-order, ordered from first to last in time
inline void scheduled_calls::insert(call_t && call)
{
    auto it = std::lower_bound(m_calls.begin(), m_calls.end(), call, [](const call_t & a, const call_t & b) { return a.m_at > b.m_at; });
    m_calls.insert(it, std::move(call));
}

inline void scheduled_calls::remove(call_t::id_t id)
{
    auto it = std::find_if(m_calls.begin(), m_calls.end(), [id](const call_t & call) { return call.m_id == id; });
    if (it != m_calls.end())
    {
        m_calls.erase(it);
    }
}

inline time_point_t scheduled_calls::next_deadline() const
{
    assert(!m_calls.empty());
    return m_calls.back().m_at;
}

inline call_t scheduled_calls::pop_and_reschedule()
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
