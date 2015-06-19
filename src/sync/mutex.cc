#include "sync/mutex.hpp"

#include "coro/coro.hpp"

namespace indecorous {

mutex_t::mutex_t() :
    m_lock(nullptr) { }

mutex_t::mutex_t(mutex_t &&other) :
        m_lock(other.m_lock),
        m_pending_locks(std::move(other.m_pending_locks)) {
    m_pending_locks.each([&] (mutex_lock_t *m) { m->m_parent = this; });
}

mutex_t::~mutex_t() {
    while (!m_pending_locks.empty()) {
        m_pending_locks.pop_front()->m_coro_cb->wait_done(wait_result_t::ObjectLost);
    }
}

mutex_lock_t mutex_t::lock() {
    return mutex_lock_t(this, nullptr);
}

mutex_lock_t mutex_t::lock(wait_object_t *interruptor) {
    return mutex_lock_t(this, interruptor);
}

mutex_lock_t::mutex_lock_t(mutex_t *parent, wait_object_t *) :
        m_parent(parent), m_coro_cb(coro_t::self()->wait_callback()) {
    if (m_parent->m_lock == nullptr) {
        m_parent->m_lock = this;
    }
    // TODO: wait and interruption
}

mutex_lock_t::mutex_lock_t(mutex_lock_t &&other) :
        intrusive_node_t<mutex_lock_t>(std::move(other)),
        m_parent(other.m_parent),
        m_coro_cb(other.m_coro_cb) {
    other.m_parent = nullptr;
    other.m_coro_cb = nullptr;
    if (m_parent->m_lock == &other) {
        m_parent->m_lock = this;
    }
}

mutex_lock_t::~mutex_lock_t() {
    if (m_parent != nullptr) {
        if (m_parent->m_lock == this) {
            if (!m_parent->m_pending_locks.empty()) {
                m_parent->m_lock = m_parent->m_pending_locks.pop_front();
                m_parent->m_lock->m_coro_cb->wait_done(wait_result_t::Success);
            } else {
                m_parent->m_lock = nullptr;
            }
        }
    }
    if (m_coro_cb != nullptr) {
        m_coro_cb->wait_done(wait_result_t::ObjectLost);
    }
}

} // namespace indecorous
