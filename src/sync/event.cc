#include "sync/event.hpp"

#include "common.hpp"
#include "errors.hpp"

namespace indecorous {

event_t::event_t(bool autoReset, bool wakeAll) :
        m_autoReset(autoReset),
        m_wakeAll(wakeAll),
        m_triggered(false) {
    assert(autoReset || wakeAll);
}

event_t::~event_t() {
    // Fail any remaining waits
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);
    }
}

bool event_t::triggered() const {
    return m_triggered;
}

bool event_t::set() {
    if (m_triggered)
        return false;

    if (m_waiters.empty()) {
        m_triggered = true;
    } else {
        if (!m_autoReset)
            m_triggered = true;

        if (m_wakeAll) {
            while (!m_waiters.empty()) {
                m_waiters.pop_front()->wait_done(wait_result_t::Success);
            }
        } else if (!m_waiters.empty()) {
            m_waiters.pop_front()->wait_done(wait_result_t::Success);
        }
    }

    return true;
}

bool event_t::reset() {
    if (!m_triggered)
        return false;
    m_triggered = false;
    return true;
}

void event_t::wait() {
    if (m_triggered) {
        assert(m_waiters.empty());
        if (m_autoReset)
            m_triggered = false;
    } else {
        coro_wait(&m_waiters);
    }
}

void event_t::addWait(wait_callback_t* cb) {
    if (m_triggered) {
        assert(m_waiters.empty());
        if (m_autoReset)
            m_triggered = false;
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void event_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

} // namespace indecorous
