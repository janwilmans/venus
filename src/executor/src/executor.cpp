/*
 * Copyright (c) 2024 Jan Wilmans
 */

#include "executor/executor.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <utility>

namespace venus {

// scheduled_call::scheduled_call() :
//     pExec(nullptr),
//     id(0)
// {
// }

// scheduled_call::scheduled_call(ExecutorBase & exec, unsigned id) :
//     pExec(&exec),
//     id(id)
// {
// }

// void scheduled_call::Cancel()
// {
//     if (pExec == nullptr)
//     {
//         return;
//     }

//     pExec->Cancel(*this);
//     pExec = nullptr;
// }

// unsigned scheduled_call::GetId() const
// {
//     return id;
// }

// bool executor::is_executor_thread() const
// {
//     return std::this_thread::get_id() == m_threadId;
// }

// void executor::run_one()
// {
//     set_executor_thread();
//     m_q.Pop()();
// }

// void executor::set_executor_thread()
// {
//     m_threadId = std::this_thread::get_id();
// }

// void executor::set_executor_thread(std::thread::id id)
// {
//     m_threadId = id;
// }

// void executor::Add(std::function<void()> fn)
// {
//     m_q.Push(fn);
// }

// void executor::Synchronize()
// {
//     assert(!is_executor_thread() && "Calling Synchronize() inside Call() will cause a deadlock");
//     SynchronizeInternally();
// }

// void executor::SynchronizeInternally()
// {
//     std::promise<bool> sync;
//     Add([&sync]() { sync.set_value(true); });
//     sync.get_future().get();
// }

// scheduled_call TimedExecutor::CallAt(const TimePoint & at, std::function<void()> fn)
// {
//     unsigned id = GetCallId();
//     Add([this, id, at, fn]() {
//         m_scheduledCalls.Insert(TimedExecutor::CallData(id, at, fn));
//     });
//     return MakeScheduledCall(id);
// }

// scheduled_call TimedExecutor::CallAfter(const Duration & interval, std::function<void()> fn)
// {
//     return CallAt(std::chrono::steady_clock::now() + interval, fn);
// }

// scheduled_call TimedExecutor::CallEvery(const Duration & interval, std::function<void()> fn)
// {
//     assert(interval > Duration::zero());

//     unsigned id = GetCallId();
//     Add([this, id, interval, fn]() {
//         m_scheduledCalls.Insert(TimedExecutor::CallData(id, std::chrono::steady_clock::now() + interval, interval, fn));
//     });
//     return MakeScheduledCall(id);
// }

// void TimedExecutor::Cancel(const scheduled_call & call)
// {
//     unsigned id = GetId(call);
//     if (is_executor_thread())
//     {
//         m_scheduledCalls.Remove(id);
//     }
//     else
//     {
//         Call([this, id]() { m_scheduledCalls.Remove(id); });
//     }
// }

// void TimedExecutor::RunOne()
// {
//     set_executor_thread();
//     if (!m_scheduledCalls.IsEmpty() && !wait_for_not_empty(m_scheduledCalls.NextDeadline()))
//     {
//         m_scheduledCalls.Pop().fn();
//     }
//     else
//     {
//         return executor::RunOne();
//     }
// }

// ActiveExecutor::ActiveExecutor() :
//     m_end(false),
//     m_thread([this] { Run(); })
// {
//     set_executor_thread(m_thread.get_id());
// }

// ActiveExecutor::~ActiveExecutor()
// {
//     Add([this] { m_end = true; });
//     m_thread.join();
// }

// void ActiveExecutor::Run()
// {
//     while (!m_end)
//     {
//         try
//         {
//             RunOne();
//         }
//         catch (std::exception & e)
//         {
//             cdbg << "executor: exception ignored: " << e.what() << "\n";
//         }
//         catch (...)
//         {
//             cdbg << "executor: exception ignored\n";
//         }
//     }
// }

} // namespace venus
