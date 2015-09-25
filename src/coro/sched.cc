#include "coro/sched.hpp"

#include <semaphore.h>

#include <cstring>
#include <numeric>

#include "coro/coro.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"
#include "utils.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_threads, shutdown_policy_t policy) :
        m_shutdown_policy(policy),
        m_shutdown(num_threads),
        m_close_flag(false),
        m_barrier(num_threads + 1),
        m_threads() {
    assert(num_threads > 0);

    // Block signals on child threads (this will be inherited)
    sigset_t sigset;
    sigset_t old_sigset;
    GUARANTEE_ERR(sigemptyset(&sigset) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGINT) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGTERM) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGPIPE) == 0);

    GUARANTEE(pthread_sigmask(SIG_BLOCK, &sigset, &old_sigset) == 0);

    for (size_t i = 0; i < num_threads; ++i) {
        m_threads.emplace_back(i, &m_shutdown, &m_barrier, &m_close_flag);
    }

    // Return SIGINT and SIGTERM to the previous state
    GUARANTEE(pthread_sigmask(SIG_SETMASK, &old_sigset, nullptr) == 0);

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

class scoped_sigaction_t {
public:
    scoped_sigaction_t() :
            m_old_sigint(),
            m_old_sigterm(),
            m_old_sigpipe(),
            m_semaphore() {
        s_instance = this;

        struct sigaction sigact;
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_flags = SA_SIGINFO;
        sigact.sa_sigaction = &scoped_sigaction_t::callback;
        sigemptyset(&sigact.sa_mask);

        GUARANTEE_ERR(sigaction(SIGINT, &sigact, &m_old_sigint) == 0);
        GUARANTEE_ERR(sigaction(SIGTERM, &sigact, &m_old_sigterm) == 0);

        sigact.sa_handler = SIG_IGN;
        sigact.sa_flags = 0;
        GUARANTEE_ERR(sigaction(SIGPIPE, &sigact, &m_old_sigpipe) == 0);

        GUARANTEE_ERR(sem_init(&m_semaphore, 0, 0) == 0);
    }

    ~scoped_sigaction_t() {
        GUARANTEE_ERR(sem_destroy(&m_semaphore) == 0);
        GUARANTEE_ERR(sigaction(SIGINT, &m_old_sigint, nullptr) == 0);
        GUARANTEE_ERR(sigaction(SIGTERM, &m_old_sigterm, nullptr) == 0);
        GUARANTEE_ERR(sigaction(SIGPIPE, &m_old_sigpipe, nullptr) == 0);
        s_instance = nullptr;
    }

    void wait() {
        eintr_wrap([&] { return sem_wait(&m_semaphore); });
    }

private:
    static thread_local scoped_sigaction_t *s_instance;

    static void callback(int, siginfo_t *, void *) {
        GUARANTEE(sem_post(&s_instance->m_semaphore) == 0);
    }

    struct sigaction m_old_sigint;
    struct sigaction m_old_sigterm;
    struct sigaction m_old_sigpipe;
    sem_t m_semaphore;
};

thread_local scoped_sigaction_t *scoped_sigaction_t::s_instance = nullptr;

void scheduler_t::run() {
    // Count the number of initial calls into the threads
    m_shutdown.reset(std::accumulate(m_threads.begin(), m_threads.end(), size_t(0),
        [] (size_t res, thread_t &t) -> size_t {
            return res + t.hub()->self_target()->m_stream.m_message_queue.size();
        }));

    switch (m_shutdown_policy) {
    case shutdown_policy_t::Eager: {
            m_barrier.wait();
            m_shutdown.shutdown(m_threads.begin()->hub());
            m_barrier.wait();
        } break;
    case shutdown_policy_t::Kill: {
            scoped_sigaction_t scoped_sigaction;
            m_barrier.wait();
            scoped_sigaction.wait();
            m_shutdown.shutdown(m_threads.begin()->hub());
            m_barrier.wait();
        } break;
    }
}

} // namespace indecorous
