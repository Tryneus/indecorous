#include "sync/event.hpp"

#include "common.hpp"
#include "errors.hpp"

namespace indecorous {

event_t::event_t(event_t &&other) :
        m_triggered(other.m_triggered),
        m_waiters(std::move(other.m_waiters)) {
    m_waiters.each([this] (auto cb) { cb->object_moved(this); });
}

// TODO: consider adding auto-reset and/or wake-one modes or separate class(es)
event_t::event_t() : m_triggered(false) { }

event_t::~event_t() {
    m_waiters.clear([] (auto cb) { cb->wait_done(wait_result_t::ObjectLost); });
}

bool event_t::triggered() const {
    return m_triggered;
}

void event_t::set() {
    m_triggered = true;
    m_waiters.clear([] (auto cb) { cb->wait_done(wait_result_t::Success); });
}

void event_t::reset() {
    m_triggered = false;
}

void event_t::add_wait(wait_callback_t* cb) {
    if (m_triggered) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void event_t::remove_wait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

} // namespace indecorous
