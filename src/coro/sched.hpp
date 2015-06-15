#ifndef CORO_SCHED_HPP_
#define CORO_SCHED_HPP_

#include <signal.h>

#include <atomic>
#include <list>

#include "coro/barrier.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"

namespace indecorous {

class dispatcher_t;

// shutdown_policy_t::Eager - return as soon as all coroutines complete
// shutdown_policy_t::Kill - begin eager shutdown after a SIGINT is received
enum class shutdown_policy_t { Eager, Kill };

class scheduler_t {
public:
    explicit scheduler_t(size_t num_threads, shutdown_policy_t policy);
    ~scheduler_t();

    std::list<thread_t> &threads();

    // Broadcasts a given callback to all the threads with no replies
    template <typename Callback, typename... Args>
    size_t broadcast_local(Args &&...args) {
        return m_threads.begin()->hub()->broadcast_local_noreply<Callback>(std::forward<Args>(args)...);
    }

    // This function will return based on the shutdown policy
    void run();

private:
    class scoped_signal_block_t {
    public:
        scoped_signal_block_t(bool enabled);
        ~scoped_signal_block_t();
        void wait() const;
    private:
        bool m_enabled;
        sigset_t m_sigset;
    } m_signal_block;

    shutdown_t m_shutdown;
    std::atomic<bool> m_close_flag;
    thread_barrier_t m_barrier;
    std::list<thread_t> m_threads;
};

} // namespace indecorous

#endif // CORO_SCHED_HPP_
