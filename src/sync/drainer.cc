#include "sync/drainer.hpp"

#include "coro/coro.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

drainer_lock_t::drainer_lock_t(drainer_t *parent) :
        m_waiters(),
        m_parent(parent) {
    assert(m_parent != nullptr);
     // Kinda silly we double-track locks, but ok I guess
    m_parent->add_lock(this);
    m_parent->add_wait(this);
}

drainer_lock_t::drainer_lock_t(drainer_lock_t &&other) :
        wait_callback_t(std::move(other)),
        intrusive_node_t<drainer_lock_t>(std::move(other)),
        m_waiters(std::move(other.m_waiters)),
        m_parent(other.m_parent) {
    other.m_parent = nullptr;
    m_waiters.each([this] (auto w) { w->object_moved(this); });
}

drainer_lock_t::drainer_lock_t(const drainer_lock_t &other) :
        wait_callback_t(),
        intrusive_node_t<drainer_lock_t>(),
        m_waiters(),
        m_parent(other.m_parent) {
    assert(m_parent != nullptr);
    m_parent->add_lock(this);
    m_parent->add_wait(this);
}

drainer_lock_t::~drainer_lock_t() {
    // TODO: this shouldn't happen except we do some sketchy shit for remote
    // tasks to clear their own drainer (by destructing their drainer lock
    // while an interruptor currently listens to it) - this is not great RAII
    m_waiters.clear([&] (auto w) { w->wait_done(wait_result_t::Success); });

    if (m_parent != nullptr) {
        m_parent->remove_lock(this);
        m_parent->remove_wait(this);
    }
}

bool drainer_lock_t::draining() const {
    assert(m_parent != nullptr);
    return m_parent->draining();
}

void drainer_lock_t::release() {
    drainer_lock_t lock(std::move(*this));
}

void drainer_lock_t::add_wait(wait_callback_t *cb) {
    if (m_parent->draining()) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void drainer_lock_t::remove_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    m_waiters.remove(cb);
}

void drainer_lock_t::wait_done(wait_result_t result) {
    m_waiters.clear([&] (auto w) { w->wait_done(result); });
}

// TODO: do we need to do anything here?
void drainer_lock_t::object_moved(waitable_t *) { }

drainer_t::drainer_t(drainer_t &&other) :
        m_locks(std::move(other.m_locks)),
        m_start_drain_waiters(std::move(other.m_start_drain_waiters)),
        m_finish_drain_waiters(std::move(other.m_finish_drain_waiters)),
        m_draining(other.m_draining) {
    other.m_draining = true;
    m_locks.each([this] (auto d) { d->m_parent = this; });
    m_start_drain_waiters.each([this] (auto w) { w->object_moved(this); });
    m_finish_drain_waiters.each([this] (auto w) { w->object_moved(this); });
}

drainer_t::drainer_t() :
        m_locks(),
        m_start_drain_waiters(),
        m_finish_drain_waiters(),
        m_draining(false) { }

drainer_t::~drainer_t() {
    drain();
}

bool drainer_t::draining() const {
    return m_draining;
}

void drainer_t::drain() {
    // We cannot allow interruption during draining, it will break assumptions
    interruptor_clear_t no_interruptor;
    m_draining = true;
    m_start_drain_waiters.clear([] (auto w) { w->wait_done(wait_result_t::Success); });
    if (!m_locks.empty()) {
        coro_t *self = coro_t::self();
        m_finish_drain_waiters.push_back(self->wait_callback());
        self->wait();
    }
}

drainer_lock_t drainer_t::lock() {
    return drainer_lock_t(this);
}

void drainer_t::add_lock(drainer_lock_t *l) {
    assert(!m_draining);
    m_locks.push_back(l);
}

void drainer_t::remove_lock(drainer_lock_t *l) {
    m_locks.remove(l);
    if (m_draining && m_locks.empty()) {
        m_finish_drain_waiters.clear([&] (auto w) { w->wait_done(wait_result_t::Success); });
    }
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
