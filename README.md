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

The Pool Executor is very good to paralelize work, scatter work, if you like.

* Single thread Executor

A single thread executor, has one queue of work that needs to be processed. It gives a hard first-come-first-serve guarentee. As soon as work is queued, tasks are picked up and processed sequentially.

Single thread Executor allows you to schedule work that is all done sequenctially, this means that all work done by the executor is inherently thread safe without any *significant* contention on locks.

There is still a lock in the queueing mechanism, but instead of having locks that protect your data structure, likely multiple locks in different places that may have to lock/unlocked several times, there is now only _one_ lock that is only touched when work is queued.











