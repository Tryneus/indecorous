#include "coro/thread.hpp"

#include <sys/time.h>

#include <limits>

#include "coro/barrier.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"

namespace indecorous {

thread_local thread_t* thread_t::s_instance = nullptr;

thread_t::thread_t(size_t _id,
                   shutdown_t *shutdown,
                   thread_barrier_t *barrier,
                   std::atomic<bool> *close_flag) :
        m_id(_id),
        m_shutdown(shutdown),
        m_barrier(barrier),
        m_close_flag(close_flag),
        m_thread(&thread_t::main, this),
        m_hub(),
        m_events(),
        m_dispatch() {
    m_thread.detach();
    m_events.add_file_wait(m_shutdown->get_file_event());
}

thread_t::~thread_t() {
    m_events.remove_file_wait(m_shutdown->get_file_event());
}

size_t thread_t::id() const {
    return m_id;
}

void thread_t::main() {
    s_instance = this;

    m_barrier->wait(); // Barrier for the scheduler_t constructor, thread ready
    m_barrier->wait(); // Wait for run or ~scheduler_t

    while (!m_close_flag->load()) {
        int64_t active_delta = m_dispatch.run();
        while (m_shutdown->update(active_delta)) {
            m_events.wait();
            active_delta = m_dispatch.run();
        }

        m_barrier->wait(); // Wait for other threads to finish
        m_barrier->wait(); // Wait for run or ~scheduler_t
    }

    m_dispatch.close();

    m_barrier->wait(); // Barrier for ~scheduler_t, safe to destruct
    s_instance = nullptr;
}

message_hub_t *thread_t::hub() {
    return &m_hub;
}

dispatcher_t *thread_t::dispatcher() {
    return &m_dispatch;
}

events_t *thread_t::events() {
    return &m_events;
}

thread_t *thread_t::self() {
    return s_instance;
}

} // namespace indecorous

