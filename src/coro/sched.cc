#include "coro/sched.hpp"

#include "coro/coro.hpp"
#include "coro/thread.hpp"

namespace indecorous {

scheduler_t* scheduler_t::s_instance = nullptr;

scheduler_t::scheduler_t(size_t num_threads) :
        m_running(false),
        m_num_threads(num_threads) {
    assert(s_instance == nullptr);

    pthread_barrier_init(&m_barrier, nullptr, m_num_threads + 1);

    // Create thread_t objects for each thread
    m_threads = new thread_t*[m_num_threads];
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i] = new thread_t(this, i, &m_barrier);
        m_thread_ids.insert(m_threads[i]->m_target.id());
    }

    // Wait for all threads to start up
    pthread_barrier_wait(&m_barrier);

    s_instance = this;
}

scheduler_t::~scheduler_t() {
    assert(s_instance == this);
    assert(!m_running);

    // Tell all threads to shutdown once we hit the following barrier
    for (size_t i = 0; i < m_num_threads; ++i) {
        assert(m_threads[i]->m_dispatch->m_active_contexts == 0);
        m_threads[i]->shutdown();
    }

    // Tell threads to exit
    pthread_barrier_wait(&m_barrier);

    // Wait for threads to exit
    pthread_barrier_wait(&m_barrier);
    pthread_barrier_destroy(&m_barrier);

    for (size_t i = 0; i < m_num_threads; ++i) {
        delete m_threads[i];
    }
    delete [] m_threads;

    s_instance = nullptr;
}

const std::unordered_set<target_id_t> &scheduler_t::all_threads() const {
    return m_thread_ids;
}

message_hub_t *scheduler_t::message_hub() {
    return &m_message_hub;
}

scheduler_t& scheduler_t::get_instance() {
    assert(s_instance != nullptr);
    return *s_instance;
}

void scheduler_t::run() {
    assert(!m_running);

    // Wait for all threads to get to their loop
    m_running = true;
    pthread_barrier_wait(&m_barrier);

    // Wait for all threads to finish all their coroutines
    pthread_barrier_wait(&m_barrier);
    m_running = false;
}
} // namespace indecorous

