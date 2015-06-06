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
                   std::atomic<bool> *exit_flag) :
        m_id(_id),
        m_hub(),
        m_events(),
        m_dispatch(),
        m_shutdown(shutdown),
        m_barrier(barrier),
        m_exit_flag(exit_flag),
        m_shutdown_event(),
        m_thread(&thread_t::main, this) {
    m_thread.detach();
}

size_t thread_t::id() const {
    return m_id;
}

void thread_t::main() {
    s_instance = this;

    m_barrier->wait(); // Barrier for the scheduler_t constructor, thread ready
    m_barrier->wait(); // Wait for run or ~scheduler_t

    while (!m_exit_flag->load()) {
        int64_t active_delta = m_dispatch.run();
        while (m_shutdown->update(active_delta)) {
            m_events.wait();
            active_delta = m_dispatch.run();
        }

        m_barrier->wait(); // Wait for other threads to finish
        m_barrier->wait(); // Wait for run or ~scheduler_t
    }

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

wait_object_t *thread_t::shutdown_event() {
    return &m_shutdown_event;
}

thread_t *thread_t::self() {
    assert(s_instance != nullptr);
    return s_instance;
}

} // namespace indecorous

