#include "sync/mutex.hpp"

#include "coro/coro.hpp"

namespace indecorous {

mutex_t::mutex_t() :
    m_lock(nullptr),
    m_pending_locks() { }

mutex_t::mutex_t(mutex_t &&other) :
        m_owner(other.m_owner),
        m_pending_locks(std::move(other.m_pending_locks)) {
    other.m_owner = nullptr;
    m_pending_locks.each([this] (auto l) { l->m_parent = this; });
}

mutex_t::~mutex_t() {
    assert(m_owner == nullptr);
    m_pending_locks.clear([] (auto l) { l->m_coro_cb->wait_done(wait_result_t::ObjectLost); });
}

mutex_acq_t mutex_t::start_acq() {
    return mutex_acq_t(this);
}

void mutex_t::add_lock(mutex_acq_t *lock) {
    m_locks.push_back(lock);
}

void mutex_t::release_lock(mutex_acq_t *lock) {
    mutex_acq_t *front = m_locks.front();
    m_locks.remove(lock);

    if (front == lock) {
        m_locks.front()->ready();
    }
}

mutex_acq_t::mutex_acq_t(mutex_t *parent) :
        m_parent(parent),
        m_waiters() {
    m_parent->m_pending_locks.push_back(this);
}

bool mutex_acq_t::has() const {
    assert(m_parent != nullptr);
    return m_parent->m_locks.front() == this;
}

void mutex_acq_t::ready() {
    assert(m_parent->m_locks.front() == this);
    m_waiters.clear([] (auto w) { w->wait_done(wait_result_t::Success); });
}

void mutex_acq_t::add_wait(wait_callback_t *cb) override final {
    if (has()) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void mutex_acq_t::remove_wait(wait_callback_t *cb) override final {
    assert(m_parent != nullptr);
    m_waiters.remove(cb);
}

mutex_acq_t::mutex_acq_t(mutex_acq_t &&other) :
        intrusive_node_t<mutex_acq_t>(std::move(other)),
        m_parent(other.m_parent),
        m_waiters(std::move(other.m_waiters)) {
    other.m_parent = nullptr;
    m_waiters.clear([] (auto w) { w->object_moved(this); });
}

mutex_acq_t::~mutex_acq_t() {
    if (m_parent != nullptr) {
        m_parent->remove_lock(this);
    }
    m_waiters.clear([] (auto w) { w->wait_done(wait_result_t::ObjectLost); });
}

} // namespace indecorous
