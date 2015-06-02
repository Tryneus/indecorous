#include "sync/event.hpp"

#include "common.hpp"
#include "errors.hpp"

namespace indecorous {

// TODO: consider adding auto-reset and/or wake-one modes or separate class(es)
event_t::event_t() : m_triggered(false) { }

event_t::~event_t() {
    // Fail any remaining waits
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);
    }
}

bool event_t::triggered() const {
    return m_triggered;
}

void event_t::set() {
    m_triggered = true;
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::Success);
    }
}

void event_t::reset() {
    m_triggered = false;
}

void event_t::wait() {
    if (!m_triggered) {
        coro_wait(&m_waiters);
    }
}

void event_t::addWait(wait_callback_t* cb) {
    if (m_triggered) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void event_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

} // namespace indecorous
