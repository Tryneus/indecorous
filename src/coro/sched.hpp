#ifndef CORO_CORO_SCHED_HPP_
#define CORO_CORO_SCHED_HPP_

#include <atomic>
#include <list>

#include "coro/barrier.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"

namespace indecorous {

class dispatcher_t;
class thread_t;

enum class shutdown_policy_t { Eager, Kill };

class scheduler_t {
public:
    explicit scheduler_t(size_t num_threads);
    ~scheduler_t();

    std::list<thread_t> &threads();

    // This function will return based on the shutdown policy
    // shutdown_policy_t::Eager - return as soon as all coroutines complete
    // shutdown_policy_t::Kill - begin Eager shutdown after a SIGINT is received
    void run(shutdown_policy_t policy);

private:
    shutdown_t m_shutdown;
    std::atomic<bool> m_close_flag;
    thread_barrier_t m_barrier;
    std::list<thread_t> m_threads;
};

} // namespace indecorous

#endif
