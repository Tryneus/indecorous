#include "sync/mutex.hpp"

namespace indecorous {

mutex_t::mutex_t() : m_locked(false) { }

mutex_t::~mutex_t() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);
    }
}

void mutex_t::unlock() {
    assert(m_locked);
    if (m_waiters.empty()) {
        m_locked = false;
    } else {
        m_waiters.pop_front()->wait_done(wait_result_t::Success);
    }
}

void mutex_t::add_wait(wait_callback_t* cb) {
    if (!m_locked) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void mutex_t::remove_wait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

mutex_lock_t::mutex_lock_t(mutex_t *parent) :
        m_parent(parent) {
    m_parent->wait();
}

mutex_lock_t::~mutex_lock_t() {
    m_parent->unlock();
}

} // namespace indecorous
