/*
 * Copyright (c) 2025 Jan Wilmans
 */

#include "executor/detail/worker_pool.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace venus {

std::size_t default_thread_count()
{
    return std::max<std::size_t>(1, std::thread::hardware_concurrency());
}

namespace detail {

namespace {

// the pool the current thread works for, nullptr on threads that are not workers
thread_local const worker_pool * t_current_pool = nullptr;

} // namespace

worker_pool::worker_pool(std::size_t thread_count, std::size_t level_count) :
    m_levels(std::max<std::size_t>(1, level_count))
{
    const auto count = std::max<std::size_t>(1, thread_count);
    m_threads.reserve(count);
    try
    {
        std::generate_n(std::back_inserter(m_threads), count, [this] { return std::thread([this] { run(); }); });
    }
    catch (...)
    {
        // creating a thread failed, stop the threads that did start before the exception leaves the constructor
        stop_and_join();
        throw;
    }
}

worker_pool::~worker_pool()
{
    assert(!is_worker_thread() && "destroying a pool from one of its own tasks will deadlock");
    wait_idle();
    stop_and_join();
}

void worker_pool::stop_and_join()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_work_available.notify_all();
    for (auto & thread : m_threads)
    {
        thread.join();
    }
}

void worker_pool::push(std::size_t level, task t)
{
    assert(level < m_levels.size());
    bool wake = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_levels[level].push_back(std::move(t));
        ++m_pending;
        // when all workers are busy, a worker will find this task when it finishes, notifying would only cost time
        wake = m_sleeping > 0;
    }

    if (wake)
    {
        m_work_available.notify_one();
    }
}

void worker_pool::wait_idle()
{
    assert(!is_worker_thread() && "calling wait_idle() from a task on the same pool will deadlock");
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle.wait(lock, [this] { return m_pending == 0; });
}

std::size_t worker_pool::thread_count() const
{
    return m_threads.size();
}

std::size_t worker_pool::level_count() const
{
    return m_levels.size();
}

bool worker_pool::is_worker_thread() const
{
    return t_current_pool == this;
}

bool worker_pool::has_work() const
{
    return std::any_of(m_levels.begin(), m_levels.end(), [](const std::deque<task> & level) { return !level.empty(); });
}

task worker_pool::pop()
{
    auto level = std::find_if(m_levels.begin(), m_levels.end(), [](const std::deque<task> & queue) { return !queue.empty(); });
    assert(level != m_levels.end());
    task result = std::move(level->front());
    level->pop_front();
    return result;
}

void worker_pool::run()
{
    t_current_pool = this;
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;)
    {
        while (!m_stop && !has_work())
        {
            ++m_sleeping;
            m_work_available.wait(lock);
            --m_sleeping;
        }

        if (!has_work())
        {
            return; // stopped and no work left
        }

        {
            task current = pop();
            lock.unlock();
            try
            {
                current();
            }
            catch (...)
            {
                // exceptions from call_async() style tasks are delivered through their futures, others are ignored.
            }
            // 'current' is destroyed here, without holding the lock, its captures may run arbitrary destructors
        }

        lock.lock();
        if (--m_pending == 0)
        {
            m_idle.notify_all();
        }
    }
}

} // namespace detail
} // namespace venus
