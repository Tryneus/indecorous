#include "coro/thread.hpp"

#include <sys/time.h>

#include <limits>

#include "coro/barrier.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"

namespace indecorous {

thread_local thread_t* thread_t::s_instance = nullptr;

thread_t::thread_t(message_hub_t *hub,
                   thread_barrier_t* barrier) :
        m_barrier(barrier),
        m_events(),
        m_dispatch(),
        m_target(hub, this),
        m_shutdown(false),
        m_thread(&thread_t::main, this) {
    m_thread.detach();
}

void thread_t::main() {
    s_instance = this;

    m_barrier->wait(); // Barrier for the scheduler_t constructor, thread ready
    m_barrier->wait(); // Wait for run or ~scheduler_t

    while (!m_shutdown) {
        while (m_dispatch.run() > 0) { m_events.wait(); }
        m_barrier->wait(); // Wait for other threads to finish
        m_barrier->wait(); // Wait for run or ~scheduler_t
    }

    m_barrier->wait(); // Barrier for ~scheduler_t, safe to destruct
}

dispatcher_t *thread_t::dispatcher() {
    return &m_dispatch;
}

local_target_t *thread_t::target() {
    return &m_target;
}

events_t *thread_t::events() {
    return &m_events;
}

void thread_t::shutdown() {
    m_shutdown = true;
}

target_id_t thread_t::id() const {
    return m_target.id();
}

thread_t *thread_t::self() {
    assert(s_instance != nullptr);
    return s_instance;
}

} // namespace indecorous

