

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace venus {

using clock_t = std::chrono::steady_clock;
using time_point_t = clock_t::time_point;
using duration_t = clock_t::duration;
using function_t = std::function<void()>;

struct call_t
{
    using id_t = std::uint32_t;
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
    [[nodiscard]] bool empty() const;
    void insert(call_t && call);
    void remove(call_t::id_t);
    [[nodiscard]] time_point_t next_deadline() const;
    call_t pop_front();

private:
    std::vector<call_t> m_calls;
};

} // namespace venus