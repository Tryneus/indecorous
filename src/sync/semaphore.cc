#include "sync/semaphore.hpp"

#include "common.hpp"

namespace indecorous {

semaphore_acq_t::semaphore_acq_t(semaphore_acq_t &&other) :
        intrusive_node_t<semaphore_acq_t>(std::move(other)),
        m_parent(other.m_parent),
        m_owned(other.m_owned),
        m_pending(other.m_pending),
        m_waiters(std::move(other.m_waiters)) {
    other.m_parent = nullptr;
    m_waiters.each([this] (auto cb) { cb->object_moved(this); });
}

semaphore_acq_t::semaphore_acq_t(size_t count, semaphore_t *parent) :
        m_parent(parent),
        m_owned(0),
        m_pending(count) {
    if (m_parent->m_available >= m_pending) {
        m_parent->m_available -= m_pending;
        m_owned = m_pending;
        m_pending = 0;
        m_parent->m_acqs.push_back(this);
    } else {
        m_parent->m_waiters.push_back(this);
    }
}

semaphore_acq_t::~semaphore_acq_t() {
    if (m_parent != nullptr) {
        if (m_pending > 0) {
            m_parent->m_waiters.remove(this);
        } else {
            m_parent->m_acqs.remove(this);
        }
        release(m_owned);
    }
}

size_t semaphore_acq_t::owned() const {
    return m_owned;
}

size_t semaphore_acq_t::pending() const {
    return m_pending;
}

void semaphore_acq_t::combine(semaphore_acq_t &&other) {
    assert(m_parent != nullptr);
    assert(m_parent == other.m_parent);
    assert(m_pending == 0);
    assert(other.m_pending == 0);

    m_owned += other.m_owned;
    other.m_parent = nullptr;

    other.m_waiters.clear([&] (auto cb) { m_waiters.push_back(cb); });
}

void semaphore_acq_t::release(size_t count) {
    assert(m_parent != nullptr);
    assert(m_owned >= count);
    m_parent->m_available += count;
    m_parent->pump_waiters();
}

void semaphore_acq_t::add_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    if (m_pending == 0) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void semaphore_acq_t::remove_wait(wait_callback_t *cb) {
    m_waiters.remove(cb);
}

semaphore_t::semaphore_t(semaphore_t &&other) :
        m_capacity(other.m_capacity),
        m_available(other.m_available),
        m_acqs(std::move(other.m_acqs)),
        m_waiters(std::move(other.m_waiters)) {
    other.m_capacity = 0;
    other.m_available = 0;
    m_acqs.each([this] (auto s) { s->m_parent = this; });
    m_waiters.each([this] (auto s) { s->m_parent = this; });
}

semaphore_t::semaphore_t(size_t _capacity) : m_capacity(_capacity), m_available(_capacity) { }

semaphore_t::~semaphore_t() {
    m_waiters.clear([] (auto s) {
            s->m_waiters.each([] (auto w) { w->wait_done(wait_result_t::ObjectLost); });
            s->m_parent = nullptr;
        });
}

size_t semaphore_t::capacity() const {
    return m_capacity;
}

size_t semaphore_t::available() const {
    return m_available;
}

void semaphore_t::extend(size_t count) {
    m_capacity += count;
    m_available += count;
    pump_waiters();
}

void semaphore_t::remove(semaphore_acq_t &&destroy) {
    assert(destroy.m_parent == this);
    if (destroy.m_pending > 0) {
        destroy.wait();
    }

    m_capacity -= destroy.m_owned;
    destroy.m_parent = nullptr;

    // Scan waiting acqs, make sure none of them want more than we now have
    DEBUG_ONLY(m_waiters.each([&] (auto s) { assert(m_capacity >= (s->m_owned + s->m_pending)); }));
}

semaphore_acq_t semaphore_t::start_acq(size_t count) {
    return semaphore_acq_t(count, this);
}

void semaphore_t::pump_waiters() {
    while (!m_waiters.empty() &&
           (m_waiters.front()->m_pending <= m_available)) {
        semaphore_acq_t *acq = m_waiters.pop_front();
        m_acqs.push_back(acq);
        m_available -= acq->m_pending;
        acq->m_waiters.each([] (auto cb) { cb->wait_done(wait_result_t::Success); });
    }
}

} // namespace indecorous

