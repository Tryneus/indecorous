#include "sync/interruptor.hpp"

#include "coro/coro.hpp"

namespace indecorous {

interruptor_t::interruptor_t(waitable_t *waitable) :
        m_triggered(false),
        m_waitable(waitable), m_waiters(),
        m_prev_waiter(this, coro_t::self()->add_interruptor(this)) { }

interruptor_t::interruptor_t(interruptor_t &&other) :
        wait_callback_t(std::move(other)),
        intrusive_node_t<interruptor_t>(std::move(other)),
        m_triggered(other.m_triggered),
        m_waitable(other.m_waitable),
        m_waiters(std::move(other.m_waiters)),
        m_prev_waiter(std::move(other.m_prev_waiter)) {
    other.m_triggered = false;
    other.m_waitable = nullptr;
}

interruptor_t::~interruptor_t() {
    coro_t::self()->remove_interruptor(this);
    m_waiters.clear([&] (auto cb) { cb->wait_done(wait_result_t::ObjectLost); });
}

bool interruptor_t::triggered() const {
    return m_triggered;
}

void interruptor_t::add_wait(wait_callback_t *cb) {
    if (m_triggered) {
        cb->wait_done(wait_result_t::Interrupted);
    } else {
        m_waiters.push_back(cb);
    }
}

void interruptor_t::remove_wait(wait_callback_t *cb) {
    m_waiters.remove(cb);
}

void interruptor_t::wait_done(DEBUG_VAR wait_result_t result) {
    assert(result == wait_result_t::Success);
    handle_triggered();
}

void interruptor_t::object_moved(waitable_t *new_ptr) {
    m_waitable = new_ptr;
}

void interruptor_t::handle_triggered() {
    m_triggered = true;
    m_waiters.clear([&] (auto cb) { cb->wait_done(wait_result_t::Interrupted); });
}


interruptor_t::prev_waiter_t::prev_waiter_t(interruptor_t *parent, waitable_t *prev_interruptor) :
        m_parent(parent),
        m_prev_interruptor(prev_interruptor) {
    if (m_prev_interruptor != nullptr) {
        m_prev_interruptor->add_wait(this);
    }
}

interruptor_t::prev_waiter_t::prev_waiter_t(prev_waiter_t &&other) :
        wait_callback_t(std::move(other)),
        m_parent(other.m_parent),
        m_prev_interruptor(other.m_prev_interruptor) {
    other.m_parent = nullptr;
    other.m_prev_interruptor = nullptr;
}

interruptor_t::prev_waiter_t::~prev_waiter_t() {
    if (m_prev_interruptor != nullptr) {
        m_prev_interruptor->remove_wait(this);
    }
}

void interruptor_t::prev_waiter_t::wait_done(DEBUG_VAR wait_result_t result) {
    assert(result == wait_result_t::Interrupted);
    m_parent->handle_triggered();
}

void interruptor_t::prev_waiter_t::object_moved(waitable_t *new_ptr) {
    m_prev_interruptor = new_ptr;
}

} // namespace indecorous
