#include "coro/sched.hpp"

#include <semaphore.h>

#include <cstring>
#include <numeric>

#include "coro/coro.hpp"
#include "coro/shutdown.hpp"
#include "coro/thread.hpp"
#include "utils.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_coro_threads, shutdown_policy_t policy) :
        m_running(false),
        m_shared_registry(),
        m_shutdown_policy(policy),
        m_shutdown(),
        m_destroying(false),
        m_barrier(num_coro_threads + 2),
        m_io_stream(),
        m_io_target(&m_io_stream),
        m_coro_threads(),
        m_io_threads() {
    construct_internal(num_coro_threads, 1);
}

scheduler_t::scheduler_t(size_t num_coro_threads, size_t num_io_threads, shutdown_policy_t policy) :
        m_running(false),
        m_shared_registry(),
        m_shutdown_policy(policy),
        m_shutdown(),
        m_destroying(false),
        m_barrier(num_coro_threads + num_io_threads + 1),
        m_io_stream(),
        m_io_target(&m_io_stream),
        m_coro_threads(),
        m_io_threads() {
    construct_internal(num_coro_threads, num_io_threads);
}

void scheduler_t::construct_internal(size_t num_coro_threads, size_t num_io_threads) {
    assert(num_coro_threads > 0);

    // Block signals on child threads (this will be inherited)
    sigset_t sigset;
    sigset_t old_sigset;
    GUARANTEE_ERR(sigemptyset(&sigset) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGINT) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGTERM) == 0);
    GUARANTEE_ERR(sigaddset(&sigset, SIGPIPE) == 0);

    GUARANTEE(pthread_sigmask(SIG_BLOCK, &sigset, &old_sigset) == 0);

    for (size_t i = 0; i < num_coro_threads; ++i) {
        m_coro_threads.emplace_back(this, &m_io_target);
    }

    for (size_t i = 0; i < num_io_threads; ++i) {
        m_io_threads.emplace_back(this, &m_io_target, &m_io_stream);
    }

    // Return SIGINT and SIGTERM to the previous state
    GUARANTEE(pthread_sigmask(SIG_SETMASK, &old_sigset, nullptr) == 0);

    // Tell the message hubs of each thread about the others
    for (auto &&t : m_coro_threads) {
        for (auto &&u : m_coro_threads) {
            t.thread()->hub()->add_local_target(u.thread()->target());
        }
    }

    m_barrier.wait(); // Wait for all threads to start up
}

scheduler_t::~scheduler_t() {
    m_destroying.store(true);
    m_barrier.wait(); // Start the threads so they can exit
    m_barrier.wait(); // Wait for threads to exit

    for (auto &&t : m_coro_threads) {
        t.thread()->join();
    }

    for (auto &&t : m_io_threads) {
        t.thread()->join();
    }
}

const std::vector<target_t *> &scheduler_t::local_targets() {
    return m_coro_threads.begin()->thread()->hub()->local_targets();
}

target_t *scheduler_t::io_target() {
    return &m_io_target;
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
    std::vector<target_t *> all_thread_targets;
    all_thread_targets.reserve(m_coro_threads.size() + m_io_threads.size());

    size_t initial_tasks = m_io_stream.size();
    for (auto &&t : m_coro_threads) {
        initial_tasks += t.thread()->queue_length();
        all_thread_targets.push_back(t.thread()->target());
    }

    for (auto &&t : m_io_threads) {
        all_thread_targets.push_back(t.thread()->target());
    }

    m_running = true;
    m_shutdown = std::make_unique<shutdown_t>(initial_tasks,
                                              std::move(all_thread_targets));

    switch (m_shutdown_policy) {
    case shutdown_policy_t::Eager: {
            m_barrier.wait();
            m_shutdown->begin_shutdown();
            m_barrier.wait();
        } break;
    case shutdown_policy_t::Kill: {
            scoped_sigaction_t scoped_sigaction;
            m_barrier.wait();
            scoped_sigaction.wait();
            m_shutdown->begin_shutdown();
            m_barrier.wait();
        } break;
    default:
        UNREACHABLE();
    }

    m_shutdown.reset();
    m_running = false;
}

} // namespace indecorous
