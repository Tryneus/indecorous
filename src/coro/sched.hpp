#ifndef CORO_SCHED_HPP_
#define CORO_SCHED_HPP_

#include <atomic>
#include <list>
#include <memory>
#include <vector>

#include "coro/barrier.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"
#include "cross_thread/shared.hpp"
#include "rpc/target.hpp"

namespace indecorous {

class dispatcher_t;

// shutdown_policy_t::Eager - return as soon as all coroutines complete
// shutdown_policy_t::Kill - begin eager shutdown after a SIGINT is received
enum class shutdown_policy_t { Eager, Kill };

class scheduler_t {
public:
    scheduler_t(size_t num_coro_threads, shutdown_policy_t policy);
    scheduler_t(size_t num_coro_threads, size_t num_io_threads, shutdown_policy_t policy);
    ~scheduler_t();

    const std::vector<target_t *> &local_targets();
    target_t *io_target();

    // Broadcasts a given callback to all coro threads with no replies
    template <typename Callback, typename... Args>
    size_t broadcast_local(Args &&...args) {
        return m_coro_threads.begin()->thread()->hub()->
            broadcast_local_noreply<Callback>(std::forward<Args>(args)...);
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

    void construct_internal(size_t num_coro_threads, size_t num_io_threads);

    bool m_running;
    shared_registry_t m_shared_registry;
    shutdown_policy_t m_shutdown_policy;
    std::unique_ptr<shutdown_t> m_shutdown;
    std::atomic<bool> m_destroying;
    thread_barrier_t m_barrier;
    io_stream_t m_io_stream;
    local_target_t<io_stream_t> m_io_target;
    std::list<coro_thread_t> m_coro_threads;
    std::list<io_thread_t> m_io_threads;
};

} // namespace indecorous

#endif // CORO_SCHED_HPP_
