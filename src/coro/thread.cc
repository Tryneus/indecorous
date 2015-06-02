#include "coro/thread.hpp"

#include <sys/time.h>

#include <limits>

#include "coro/barrier.hpp"
#include "coro/sched.hpp"
#include "errors.hpp"

namespace indecorous {

thread_local thread_t* thread_t::s_instance = nullptr;

thread_t::thread_t(shutdown_t *shutdown,
                   thread_barrier_t *barrier,
                   std::atomic<bool> *exit_flag) :
        m_hub(),
        m_events(),
        m_dispatch(),
        m_target(this),
        m_shutdown(shutdown),
        m_barrier(barrier),
        m_exit_flag(exit_flag),
        m_shutdown_event(),
        m_thread(&thread_t::main, this) {
    m_thread.detach();
}

void thread_t::main() {
    s_instance = this;

    m_barrier->wait(); // Barrier for the scheduler_t constructor, thread ready
    m_barrier->wait(); // Wait for run or ~scheduler_t

    while (!m_exit_flag->load()) {
        int64_t active_delta = 0;
        while (m_shutdown->update(active_delta)) {
            active_delta = m_dispatch.run();
            m_events.wait();
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

local_target_t *thread_t::target() {
    return &m_target;
}

events_t *thread_t::events() {
    return &m_events;
}

wait_object_t *thread_t::shutdown_event() {
    return &m_shutdown_event;
}

target_id_t thread_t::id() const {
    return m_target.id();
}

thread_t *thread_t::self() {
    assert(s_instance != nullptr);
    return s_instance;
}

bool thread_t::operator ==(const thread_t &other) const {
    return id() == other.id();
}

} // namespace indecorous

