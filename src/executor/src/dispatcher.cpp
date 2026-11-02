/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include "executor/dispatcher.hpp"

#include <deque>
#include <mutex>
#include <utility>

namespace venus {

namespace {

const std::size_t priority_levels = 3;

// maximum number of tasks a serial queue executes before it yields its thread to other work of the same priority
const int serial_batch_size = 16;

std::size_t to_level(priority p)
{
    return static_cast<std::size_t>(p);
}

} // namespace

struct serial_queue::state
{
    state(detail::worker_pool & pool, std::size_t level) :
        m_pool(&pool),
        m_level(level)
    {
    }

    detail::worker_pool * m_pool;
    std::size_t m_level;

    std::mutex m_mutex;
    std::deque<task> m_tasks;
    bool m_scheduled = false; // true while a drain job is queued or running for this queue
};

namespace {

void submit_drain(std::shared_ptr<serial_queue::state> state);

// runs up to serial_batch_size tasks, then re-submits itself behind the other work of the same priority
void drain(const std::shared_ptr<serial_queue::state> & state)
{
    for (int i = 0; i < serial_batch_size; ++i)
    {
        task current;
        {
            std::lock_guard<std::mutex> lock(state->m_mutex);
            if (state->m_tasks.empty())
            {
                state->m_scheduled = false;
                return;
            }
            current = std::move(state->m_tasks.front());
            state->m_tasks.pop_front();
        }

        try
        {
            current();
        }
        catch (...)
        {
            // exceptions from dispatch_async() tasks are delivered through their futures, others are ignored.
        }
    }

    {
        std::lock_guard<std::mutex> lock(state->m_mutex);
        if (state->m_tasks.empty())
        {
            state->m_scheduled = false;
            return;
        }
    }
    submit_drain(state);
}

void submit_drain(std::shared_ptr<serial_queue::state> state)
{
    auto * pool = state->m_pool;
    const auto level = state->m_level;
    pool->push(level, [queue = std::move(state)] { drain(queue); });
}

} // namespace

serial_queue::serial_queue(std::shared_ptr<state> shared_state) :
    m_state(std::move(shared_state))
{
}

void serial_queue::dispatch(task t)
{
    bool schedule = false;
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_tasks.push_back(std::move(t));
        schedule = !m_state->m_scheduled;
        m_state->m_scheduled = true;
    }

    if (schedule)
    {
        submit_drain(m_state);
    }
}

dispatcher::dispatcher(std::size_t thread_count) :
    m_pool(thread_count, priority_levels)
{
}

void dispatcher::dispatch(task t, priority p)
{
    m_pool.push(to_level(p), std::move(t));
}

serial_queue dispatcher::make_serial_queue(priority p)
{
    return serial_queue(std::make_shared<serial_queue::state>(m_pool, to_level(p)));
}

void dispatcher::wait_idle()
{
    m_pool.wait_idle();
}

std::size_t dispatcher::thread_count() const
{
    return m_pool.thread_count();
}

} // namespace venus
