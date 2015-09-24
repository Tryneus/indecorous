#include "sync/drainer.hpp"

#include "coro/coro.hpp"

namespace indecorous {

drainer_lock_t::drainer_lock_t(drainer_t *parent) :
        m_parent(parent) {
    assert(m_parent != nullptr);
    assert(!m_parent->draining()); // Cannot acquire new locks on a drainer being destroyed
    m_parent->m_locks.push_back(this);
}

drainer_lock_t::drainer_lock_t(drainer_lock_t &&other) :
        intrusive_node_t<drainer_lock_t>(std::move(other)),
        m_parent(other.m_parent) {
    other.m_parent = nullptr;
}

drainer_lock_t::drainer_lock_t(const drainer_lock_t &other) :
        intrusive_node_t<drainer_lock_t>(),
        m_parent(other.m_parent) {
    assert(m_parent != nullptr);
    m_parent->m_locks.push_back(this);
}

drainer_lock_t::~drainer_lock_t() {
    if (m_parent != nullptr) {
        m_parent->m_locks.remove(this);
        if (m_parent->draining() && m_parent->m_locks.empty()) {
            m_parent->m_finish_drain_waiter->wait_done(wait_result_t::Success);
        }
    }
}

bool drainer_lock_t::draining() const {
    return m_parent->draining();
}

void drainer_lock_t::add_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    m_parent->add_wait(cb);
}

void drainer_lock_t::remove_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    m_parent->remove_wait(cb);
}

drainer_t::drainer_t(drainer_t &&other) :
        m_locks(std::move(other.m_locks)),
        m_start_drain_waiters(std::move(other.m_start_drain_waiters)),
        m_finish_drain_waiter(other.m_finish_drain_waiter) {
    assert(m_finish_drain_waiter == nullptr);
    m_locks.each([this] (auto d) { d->m_parent = this; });
    m_start_drain_waiters.each([this] (auto w) { w->object_moved(this); });
}

drainer_t::drainer_t() : m_finish_drain_waiter(nullptr) { }

drainer_t::~drainer_t() {
    assert(m_finish_drain_waiter == nullptr);
    coro_t *self = coro_t::self();
    m_finish_drain_waiter = self->wait_callback();
    if (!m_locks.empty()) {
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
