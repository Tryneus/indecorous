#include "coro/thread.hpp"

#include <signal.h>
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
}

size_t thread_t::id() const {
    return m_id;
}

void thread_t::join() {
    m_thread.join();
}

void thread_t::main() {
    s_instance = this;

    m_barrier->wait(); // Barrier for the scheduler_t constructor, thread ready
    m_barrier->wait(); // Wait for run or ~scheduler_t

    while (!m_close_flag->load()) {
        m_stop_immediately = false;
        m_shutdown->update(m_dispatch.run(), &m_hub);
        while (!m_stop_immediately) {
            m_events.wait();
            m_shutdown->update(m_dispatch.run(), &m_hub);
        }

        assert(m_hub.self_target()->m_stream.message_queue.size() == 0);
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

