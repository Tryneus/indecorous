#include "sync/drainer.hpp"

#include "coro/coro.hpp"

namespace indecorous {

drainer_lock_t::drainer_lock_t(drainer_t *parent) :
        m_parent(parent) {
    assert(!m_parent->draining()); // Cannot acquire new locks on a drainer being destroyed
    ++m_parent->m_outstanding_locks;
}

drainer_lock_t::drainer_lock_t(drainer_lock_t &&other) :
        m_parent(other.m_parent) { }

drainer_lock_t::drainer_lock_t(const drainer_lock_t &other) :
        m_parent(other.m_parent) {
    assert(!m_parent->draining()); // Cannot acquire new locks on a drainer being destroyed
    ++m_parent->m_outstanding_locks;
}

drainer_lock_t::~drainer_lock_t() {
    if (--m_parent->m_outstanding_locks == 0 && m_parent->draining()) {
        m_parent->m_finish_drain_waiter->wait_done(wait_result_t::Success);
    }
}

bool drainer_lock_t::draining() const {
    return m_parent->draining();
}

void drainer_lock_t::add_wait(wait_callback_t *cb) {
    m_parent->add_wait(cb);
}

void drainer_lock_t::remove_wait(wait_callback_t *cb) {
    m_parent->remove_wait(cb);
}

drainer_t::drainer_t() :
    m_outstanding_locks(0) { }

drainer_t::~drainer_t() {
    coro_t *self = coro_t::self();
    m_finish_drain_waiter = self->wait_callback();
    if (m_outstanding_locks != 0) {
        self->wait();
    }
}

bool drainer_t::draining() const {
    return m_finish_drain_waiter != nullptr;
}

drainer_lock_t drainer_t::lock() {
    return drainer_lock_t(this);
}

void drainer_t::add_wait(wait_callback_t *cb) {
    if (draining()) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_start_drain_waiters.push_back(cb);
    }
}

void drainer_t::remove_wait(wait_callback_t *cb) {
    m_start_drain_waiters.remove(cb);
}

} // namespace indecorous
