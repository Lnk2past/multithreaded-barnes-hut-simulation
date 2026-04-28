#pragma once

#include <barrier>
#include <thread>
#include <vector>

/**
 * A thread synchronization wrapper that keeps any number of callables in lockstep with eachother.
 * A pair of std::barrier's are used to keep threads synchronized with a "driver" thread calling
 * Syncable::trigger
 */
struct Syncable
{
    std::size_t num_threads {1};          // number of threads
    std::barrier<> sync_point_1 {1};      // synchronization mechanism to start all work on threads for a single step
    std::barrier<> sync_point_2 {1};      // synchronization mechanism to wait for all work to complete for a single step
    std::vector<std::jthread> threads{};  // thread pool

    /**
     * Initializes mechanisms with the number of threads specified.
     * 
     * Arguments:
     *     nthreads: number of threads to use
     */
    Syncable(const std::size_t nthreads):
        num_threads(nthreads),
        sync_point_1(nthreads+1),
        sync_point_2(nthreads+1)
    {
        threads.reserve(num_threads);
    }

    /**
     * Destructor to disable the lock and unblock the threads to synchronize.
     */
    ~Syncable()
    {
        request_stop();
        sync_point_1.arrive_and_drop();
        sync_point_2.arrive_and_drop();
    }

    /**
     * Releases the barrier to execute all pending threads and wait for each thread to complete.
     */
    auto trigger() -> void
    {
        sync_point_1.arrive_and_wait();
        sync_point_2.arrive_and_wait();
    }

    /**
     * Request a stop to the threads; might be excessive to provide this but :shrug:
     */
    auto request_stop() -> void
    {
        for (auto& t: threads)
        {
            t.request_stop();
        }
    }
};
