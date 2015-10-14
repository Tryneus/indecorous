#include "coro/thread.hpp"

#include <signal.h>
#include <sys/time.h>

#include <limits>

#include "coro/barrier.hpp"
#include "coro/sched.hpp"
#include "coro/shutdown.hpp"
#include "errors.hpp"

namespace indecorous {

thread_local thread_t* thread_t::s_instance = nullptr;

thread_t::thread_t(size_t _id, scheduler_t *parent) :
        m_id(_id),
        m_parent(parent),
        m_thread(&thread_t::main, this),
        m_stop_event(),
        m_stop_immediately(false),
        m_hub(),
        m_events(),
        m_dispatch() {
}

size_t thread_t::id() const {
    return m_id;
}

shared_registry_t *thread_t::get_shared_registry() {
    return &m_parent->m_shared_registry;
}

shutdown_t *thread_t::get_shutdown() {
    return &m_parent->m_shutdown;
}

void thread_t::join() {
    m_thread.join();
}

void thread_t::main() {
    s_instance = this;

    m_parent->m_barrier.wait(); // Barrier for the scheduler_t constructor, thread ready
    m_parent->m_barrier.wait(); // Wait for run or ~scheduler_t

    while (!m_parent->m_close_flag.load()) {
        m_stop_immediately = false;
        m_parent->m_shutdown.update(m_dispatch.run(), &m_hub);
        while (!m_stop_immediately) {
            m_events.check(m_dispatch.m_run_queue.empty());
            m_parent->m_shutdown.update(m_dispatch.run(), &m_hub);
        }

        assert(m_hub.self_target()->m_stream.m_message_queue.size() == 0);
        m_parent->m_barrier.wait(); // Wait for other threads to finish
        m_parent->m_barrier.wait(); // Wait for run or ~scheduler_t
    }

    m_dispatch.close();

    m_parent->m_barrier.wait(); // Barrier for ~scheduler_t, safe to destruct
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

