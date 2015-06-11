#include "coro/sched.hpp"

#include <numeric>

#include "coro/coro.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_threads) :
        m_close_flag(false),
        m_barrier(num_threads + 1) {
    for (size_t i = 0; i < num_threads; ++i) {
        m_threads.emplace_back(i, &m_shutdown, &m_barrier, &m_close_flag);
    }

    // Tell the message hubs of each thread about the others
    for (auto &&t : m_threads) {
        t.hub()->set_local_targets(&m_threads);
    }

    m_barrier.wait(); // Wait for all threads to start up
}

scheduler_t::~scheduler_t() {
    m_close_flag.store(true);
    m_barrier.wait(); // Start the threads so they can exit
    m_barrier.wait(); // Wait for threads to exit
}

std::list<thread_t> &scheduler_t::threads() {
    return m_threads;
}

void scheduler_t::run(shutdown_policy_t policy) {
    // Count the number of initial calls into the threads
    m_shutdown.reset(std::accumulate(m_threads.begin(), m_threads.end(), size_t(0),
        [] (size_t res, thread_t &t) -> size_t {
            return res + t.hub()->self_target()->m_stream.message_queue.size();
        }));

    m_barrier.wait(); // Wait for all threads to get to their loop

    switch (policy) {
    case shutdown_policy_t::Eager: // Shutdown immediately
        break;
    case shutdown_policy_t::Kill:  // TODO: Shutdown after SIGINT
        break;
    }

    if (m_threads.size() > 0) {
        m_shutdown.shutdown(m_threads.begin()->hub());
    }

    m_barrier.wait(); // Wait for all threads to finish all their coroutines
}

} // namespace indecorous
