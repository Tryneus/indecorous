#include "coro/sched.hpp"

#include "coro/coro.hpp"
#include "coro/thread.hpp"

namespace indecorous {

scheduler_t::scheduler_t(size_t num_threads) :
        m_running(false),
        m_num_threads(num_threads),
        m_barrier(m_num_threads + 1) {
    // Create thread_t objects for each thread
    m_threads = new thread_t*[m_num_threads];
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i] = new thread_t(&m_message_hub, &m_barrier);
        m_thread_ids.insert(m_threads[i]->id());
    }

    m_barrier.wait(); // Wait for all threads to start up
}

scheduler_t::~scheduler_t() {
    assert(!m_running);

    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i]->exit();
    }

    m_barrier.wait(); // Start the threads so they can exit
    m_barrier.wait(); // Wait for threads to exit

    for (size_t i = 0; i < m_num_threads; ++i) {
        delete m_threads[i];
    }
    delete [] m_threads;
}

const std::unordered_set<target_id_t> &scheduler_t::all_threads() const {
    return m_thread_ids;
}

message_hub_t *scheduler_t::message_hub() {
    return &m_message_hub;
}

void scheduler_t::run() {
    assert(!m_running);

    shutdown_t shutdown(m_num_threads); // Control when the threads stop
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i]->set_shutdown_context(&shutdown);
    }

    m_running = true;
    m_barrier.wait(); // Wait for all threads to get to their loop
    m_barrier.wait(); // Wait for all threads to finish all their coroutines
    m_running = false;

    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i]->set_shutdown_context(nullptr);
    }
}

} // namespace indecorous
