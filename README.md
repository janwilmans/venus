# venus

Venus is yet another executor library, why? because I needed something simple and I couldn't find anything oneline.

The objective is to solve these common problems, one or more of these can apply:

* you have a data set that needs to be modified by multiple actors, possibly on different threads
* you need to parallizable work
* result(s) needs to be synchronized/sequentilized periodically
* you need to dispatch work into the background and have callback with the results

The main tools we'll be using are:
* executors (more details below)
* queues
* mutexes 
* threads

If this sounds too trivial, that is good, because that's what I'm aiming for.

## Queues, Mutexes and Threads

These are available from C++14 and we will use them as such.

## Executors

Executors are not part of the C++ standard library, so we will implement them.
Venus' Executors are not exactly the same idea as the 'executor proposal' or the 'paralel STL', I still used the name because I think its fits well. There are two kinds of executors available here:

* Pool Executor

A pool executor, has one queue of work that needs to be processed. It gives no guarentees on when it will be processed, or in what order. You give it a number of threads to use and tasks will be picked up on a best effort basis. As soon as a working thread is done, and there is work in the queue, it is picked up and processed.

A Pool Executor supports Parallelism, not to be confused with Concerrency. In Parallelism tasks are by definition independent and do not access the same data. This is why it provides no 'thread safety' features what so ever.

The goal of this executor is to utilize the multitude of cores in the CPU, you have tasks that need doing, let's get them done as soon as possible.

The Pool Executor is very good to paralelize work, scatter work, if you like.

* Single thread Executor

A single thread executor, has one queue of work that needs to be processed. It gives a hard first-come-first-serve guarentee. As soon as work is queued, tasks are picked up and processed sequentially.

Single thread Executor allows you to schedule work that is all done sequenctially, this means that all work done by the executor is inherently thread safe without any *significant* contention on locks.

There is still a lock in the queueing mechanism, but instead of having locks that protect your data structure, likely multiple locks in different places that may have to lock/unlocked several times, there is now only _one_ lock that is only touched when work is queued.

The Single thread Executor is used to synchronize work, gather tasks, so to say.

## Effective use of Venus Executors

Both types of executors are intended to work together. If you have paralel work and you need to access data that other tasks can also access you might be temped to add a synchronization primitive like a Mutex. However, if the task you queue on Pool Executor can block, you risk blocking other tasks and 'creating an idle core' while other work could be done.

Instead, use this strategy:

* first schedule a task on a Single thread Executor and either copy the data to work on or just do the work if its a small task.
* at the end of that task, schedule the paralel work on a Pool Executor
* at the _end_ of the paralel work, schedule a task back on the Single thread Executor to synchronously process the result.

![image](https://user-images.githubusercontent.com/5933444/176538261-412266f9-ad0f-4fb8-8c6f-4ab8f86ae733.png)

Guidelines:

* tasks on the Pool Executor should not take locks or do blocking I/O (nor should they need to)
* its OK for tasks on the Pool Executor to take a long time
* tasks on the Single thread Executor block all other tasks in its queue, so keep these tasks are short as possible.




