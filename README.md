# venus

Venus is yet another executor library, why? because I needed something simple and I couldn't find anything online.

The objective is to solve these common problems, one or more of these can apply:

-   you have a data set that needs to be modified by multiple actors, possibly on different threads
-   you need to parallelize work
-   result(s) need to be synchronized/sequentialized periodically
-   you need to dispatch work into the background and have a callback with the results

The main tools we'll be using are:

-   executors (more details below)
-   queues
-   std::mutex
-   std::threads
-   std::condition_variable

If this sounds too trivial, that is good, because that's what I'm aiming for.

## Requirements

-   Queues, Mutexes and Threads
-   These are available from C++14 and we will use them as such, there are no dependencies other than the standard library.
-   Tested with GCC 14.2.0 and MSVC 19.44 (Visual Studio 2022); CI builds with GCC, Clang, AppleClang and MSVC. The single thread executor was tested with GCC 4.9.2, the pool executor and dispatcher only use C++14 features that GCC 4.9 supports, but were not tested with it yet.
-   a modern cmake version is required (3.18), as low as 3.11 might work as long as FetchContent is supported, but I did not try this. I tested with cmake version [3.31.20250103-gb779fcf](https://github.com/Kitware/CMake/tree/b779fcf6a048ffe8a87eb07f6abd69fd9cedd13f) on GCC 4.9.2 and that compiled out of the box.

## Using venus in your project

```cmake
add_subdirectory(venus/src venus)
target_link_libraries(my_target PRIVATE venus::executor)
```

When venus is used as a subproject, its tests are not built (so googletest is not fetched) and it is not compiled with extra warnings or `-Werror`.

| CMake option        | Default                    | Meaning                                                              |
| ------------------- | -------------------------- | -------------------------------------------------------------------- |
| `VENUS_BUILD_TESTS` | ON only as top-level build | build the tests, fetches googletest                                  |
| `VENUS_WARNINGS`    | ON only as top-level build | extra warnings, and warnings as errors (unless `CI_SERVER` is set)   |
| `VENUS_SANITIZE`    | empty                      | sanitizers to enable, for example `address,undefined` or `thread`    |

## Executors

Executors are not part of the C++ standard library, so we will implement them.
Venus' Executors are not exactly the same idea as the 'executor proposal' or the 'parallel STL', I still used the name because I think it fits well. There are three kinds of executors available here:

### Pool executor (venus::pool_executor)

A pool executor has one queue of work that needs to be processed. It gives no guarantees on when it will be processed, or in what order. You give it a number of threads to use (by default, one per core) and tasks will be picked up on a best effort basis. As soon as a working thread is done, and there is work in the queue, it is picked up and processed.

A Pool executor supports Parallelism, not to be confused with Concurrency. In Parallelism, tasks are by definition independent and do not access the same data. This is why it provides no 'thread safety' features whatsoever.

The goal of this executor is to utilize the multitude of cores in the CPU, you have tasks that need doing, let's get them done as soon as possible.

The Pool executor is very good to parallelize work, scatter work, if you like.

```cpp
venus::pool_executor pool;
pool.add([] { /* independent work */ });
auto result = pool.call_async([] { return 42; }); // std::future<int>
pool.wait_idle();
```

### Single thread executor (venus::executor)

A single thread executor has one queue of work that needs to be processed. It gives a hard first-come-first-serve guarantee. As soon as work is queued, tasks are picked up and processed sequentially.

Single thread executor allows you to schedule work that is all done sequentially, this means that all work done by the executor is inherently thread safe without any _significant_ contention on locks.

There is still a lock in the queueing mechanism, but instead of having locks that protect your data structure, likely multiple locks in different places that may have to be locked/unlocked several times, there is now only _one_ lock that is only touched when work is queued.

The Single thread executor is used to synchronize work, gather tasks, so to say.

It also runs timers: `call_at`, `call_after` and `call_every`. Queued tasks always go before timers, so a continuous flood of tasks delays timers.

### Dispatcher (venus::dispatcher)

The dispatcher combines both ideas, inspired by [Grand Central Dispatch](https://en.wikipedia.org/wiki/Grand_Central_Dispatch), but without depending on it. It takes the parts that give most of the benefit:

-   one fixed pool of threads, one per core by default, shared by all producers
-   three priorities: `high`, `normal` and `low`; a thread always picks the oldest task of the highest priority
-   `dispatch()` runs tasks in parallel, like the pool executor
-   `make_serial_queue()` creates serial queues: tasks of one serial queue run one at a time, in order, like on a single thread executor. A serial queue is not a thread, it is a cheap object, so you can have one per producer, data structure or subsystem, and they all share the cores.

A serial queue with work occupies at most one thread at a time, and after a batch of 16 tasks it goes to the back of its priority, so one busy serial queue does not starve the others. Higher priority work does always go first.

```cpp
venus::dispatcher dispatcher;
auto model = dispatcher.make_serial_queue(venus::priority::high);

dispatcher.dispatch([&] {
    auto result = heavy_computation();                          // runs in parallel
    model.dispatch([&, result] { apply_to_model(result); });    // runs serialized, no locks needed
}, venus::priority::low);
```

The dispatcher must outlive its serial queues. Tasks that were dispatched to a serial queue still run when the serial queue object is destroyed.

### Exceptions and lifetime

-   Exceptions thrown by tasks started with `call()`, `call_async()` and `dispatch_async()` are delivered through the returned future. Exceptions thrown by other tasks are caught and ignored, they do not stop the executor.
-   The destructors of `pool_executor` and `dispatcher` wait until all tasks, including tasks added by tasks, are finished. The destructor of `executor` runs the tasks that were queued before it, and discards timers that have not run yet.
-   Using an executor while it is being destroyed violates the C++ object lifetime rules. A `scheduled_call` handle must not be used after its executor is destroyed.

### Timers and clock adjustments

Timers are based on `std::chrono::steady_clock` and should not be affected by adjustments of the system clock. However, libstdc++ before GCC 10 waits on `system_clock` internally, so on those compilers adjusting the system clock can make a timer fire early or late.

## Effective use of Venus Executors

The executors are intended to work together. If you have parallel work and you need to access data that other tasks can also access you might be tempted to add a synchronization primitive like a Mutex. However, if the task you queue on a Pool executor can block, you risk blocking other tasks and 'creating an idle core' while other work could be done.

Instead, use this strategy:

-   first schedule a task on a Single thread executor (or serial queue) and either copy the data to work on or just do the work if it's a small task.
-   at the end of that task, schedule the parallel work on a Pool executor (or dispatcher)
-   at the _end_ of the parallel work, schedule a task back on the Single thread executor (or serial queue) to synchronously process the result.

![image](https://user-images.githubusercontent.com/5933444/176538261-412266f9-ad0f-4fb8-8c6f-4ab8f86ae733.png)

Guidelines:

-   tasks on the Pool executor should not take locks or do blocking I/O (nor should they need to)
-   it's OK for tasks on the Pool executor to take a long time
-   tasks on the Single thread executor block all other tasks in its queue, so keep these tasks as short as possible.

References:

-   https://arxiv.org/pdf/2309.04259
