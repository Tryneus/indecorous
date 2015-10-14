#ifndef CORO_SCHED_HPP_
#define CORO_SCHED_HPP_

#include <atomic>
#include <list>

#include "coro/barrier.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"
#include "cross_thread/shared.hpp"

namespace indecorous {

class dispatcher_t;

// shutdown_policy_t::Eager - return as soon as all coroutines complete
// shutdown_policy_t::Kill - begin eager shutdown after a SIGINT is received
enum class shutdown_policy_t { Eager, Kill };

class scheduler_t {
public:
    scheduler_t(size_t num_threads, shutdown_policy_t policy);
    ~scheduler_t();

    std::list<thread_t> &threads();

    // Broadcasts a given callback to all the threads with no replies
    template <typename Callback, typename... Args>
    size_t broadcast_local(Args &&...args) {
        return m_threads.begin()->hub()->broadcast_local_noreply<Callback>(std::forward<Args>(args)...);
    }

    // Creates a shared object of the given type that can be sent to local targets.
    // The returned value controls the lifetime of this object.
    // Can only be called while the scheduler is not running - the registry is not thread-safe.
    template <typename T, typename ...Args>
    shared_var_t<T> create_shared(Args &&...args) {
        assert(!m_running);
        return shared_var_t<T>(&m_shared_registry, std::forward<Args>(args)...);
    }

    // This function will return based on the shutdown policy
    void run();

private:
    // Threads will use several of these members to synchronize by
    friend class thread_t;

    bool m_running;
    shared_registry_t m_shared_registry;
    shutdown_policy_t m_shutdown_policy;
    shutdown_t m_shutdown;
    std::atomic<bool> m_close_flag;
    thread_barrier_t m_barrier;
    std::list<thread_t> m_threads;
};

} // namespace indecorous

#endif // CORO_SCHED_HPP_
