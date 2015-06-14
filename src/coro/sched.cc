#include "coro/sched.hpp"

#include <numeric>

#include "coro/coro.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_threads) :
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

class scoped_signal_block_t {
public:
    scoped_signal_block_t(int _signum) : m_signum(_signum) {
        int res = sigemptyset(&m_sigset);
        assert(res == 0);
        res = sigaddset(&m_sigset, m_signum);
        assert(res == 0);

        sigset_t old_sigset;
        res = sigprocmask(SIG_BLOCK, sigset(), &old_sigset);
        assert(res == 0);

        // Make sure no one else is messing with signals - could lead to a race condition
        assert(sigismember(&old_sigset, m_signum) == 0);
    }

    ~scoped_signal_block_t() {
        int res = sigprocmask(SIG_UNBLOCK, sigset(), nullptr);
        assert(res == 0);
    }

    int signum() const {
        return m_signum;
    }

    const sigset_t *sigset() const {
        return &m_sigset;
    }

private:
    int m_signum;
    sigset_t m_sigset;
};

void wait_for_signal(const scoped_signal_block_t &sigblock) {
    int res = sigwaitinfo(sigblock.sigset(), nullptr);
    assert(res == sigblock.signum());
}

void scheduler_t::run(shutdown_policy_t policy) {
    // Block SIGINT from being handled normally
    scoped_signal_block_t sigint_block(SIGINT);

    // Count the number of initial calls into the threads
    m_shutdown.reset(std::accumulate(m_threads.begin(), m_threads.end(), size_t(0),
        [] (size_t res, thread_t &t) -> size_t {
            return res + t.hub()->self_target()->m_stream.message_queue.size();
        }));

    m_barrier.wait(); // Wait for all threads to get to their loop

    switch (policy) {
    case shutdown_policy_t::Eager:
        // Shutdown immediately
        break;
    case shutdown_policy_t::Kill:
        wait_for_signal(sigint_block);
        break;
    }

    if (m_threads.size() > 0) {
        m_shutdown.shutdown(m_threads.begin()->hub());
    }

    m_barrier.wait(); // Wait for all threads to finish all their coroutines
}

} // namespace indecorous
