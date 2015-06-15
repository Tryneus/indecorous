#include "coro/sched.hpp"

#include <numeric>

#include "coro/coro.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_threads, shutdown_policy_t policy) :
        m_signal_block(policy == shutdown_policy_t::Kill),
        m_shutdown(num_threads),
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

    for (auto &&t : m_threads) {
        t.join();
    }
}

std::list<thread_t> &scheduler_t::threads() {
    return m_threads;
}

void scheduler_t::run() {
    // Count the number of initial calls into the threads
    m_shutdown.reset(std::accumulate(m_threads.begin(), m_threads.end(), size_t(0),
        [] (size_t res, thread_t &t) -> size_t {
            return res + t.hub()->self_target()->m_stream.message_queue.size();
        }));

    m_barrier.wait(); // Wait for all threads to get to their loop

    // This is a no-op if we are in Eager shutdown mode
    m_signal_block.wait();

    if (m_threads.size() > 0) {
        m_shutdown.shutdown(m_threads.begin()->hub());
    }

    m_barrier.wait(); // Wait for all threads to finish all their coroutines
}

scheduler_t::scoped_signal_block_t::scoped_signal_block_t(bool enabled) :
        m_enabled(enabled) {
    if (m_enabled) {
        int res = sigemptyset(&m_sigset);
        assert(res == 0);
        res = sigaddset(&m_sigset, SIGINT);
        assert(res == 0);
        res = sigaddset(&m_sigset, SIGTERM);
        assert(res == 0);

        res = pthread_sigmask(SIG_BLOCK, &m_sigset, &m_old_sigset);
        assert(res == 0);

        // Make sure no one else is messing with signals - could lead to a race condition
        assert(sigismember(&old_sigset, SIGINT) == 0);
        assert(sigismember(&old_sigset, SIGTERM) == 0);
    }
}

scheduler_t::scoped_signal_block_t::~scoped_signal_block_t() {
    if (m_enabled) {
        int res = pthread_sigmask(SIG_UNBLOCK, &m_sigset, nullptr);
        assert(res == 0);
    }
}

void scheduler_t::scoped_signal_block_t::wait() const {
    if (m_enabled) {
        int res = sigwaitinfo(&m_sigset, nullptr);
        assert(res == SIGINT || res == SIGTERM);
    }
}

} // namespace indecorous
