#include "sync/promise.hpp"

namespace indecorous {

// Specialization of these classes for void
future_t<void>::future_t(future_t<void> &&other) :
        intrusive_node_t<future_t<void> >(std::move(other)),
        m_data(other.m_data),
        m_waiters(std::move(other.m_waiters)) {
    other.m_data = nullptr;
}

future_t<void>::future_t(promise_data_t<void> *data) :
        m_data(data),
        m_waiters() { }

future_t<void>::~future_t() {
    if (m_data != nullptr && m_data->remove_future(this)) {
        delete m_data;
    }
}

bool future_t<void>::has() const {
    GUARANTEE(m_data != nullptr);
    return m_data->has();
}

void future_t<void>::add_wait(wait_callback_t *cb) {
    GUARANTEE(m_data != nullptr);
    if (m_data->has()) {
        cb->wait_done(wait_result_t::Success);
    } else if (m_data->abandoned()) {
        cb->wait_done(wait_result_t::ObjectLost);
    } else {
        m_waiters.push_back(cb);
    }
}
void future_t<void>::remove_wait(wait_callback_t *cb) {
    m_waiters.remove(cb);
}

void future_t<void>::notify(wait_result_t result) {
    m_waiters.clear([result] (auto cb) { cb->wait_done(result); });
}

promise_data_t<void>::promise_data_t() :
        m_fulfilled(false),
        m_abandoned(false),
        m_futures(),
        m_chain() { }

promise_data_t<void>::~promise_data_t() {
    m_chain.clear([] (auto p) { delete p; });
}

bool promise_data_t<void>::has() const { return m_fulfilled; }
bool promise_data_t<void>::abandoned() const { return m_abandoned; }

void promise_data_t<void>::assign() {
    GUARANTEE(m_fulfilled == false);
    m_fulfilled = true;
    notify_all();
}

future_t<void> promise_data_t<void>::add_future() {
    future_t<void> res(this);
    m_futures.push_back(&res);
    return res;
}

void promise_data_t<void>::notify_all() {
    if (m_fulfilled) {
        // TODO: this will leak if `handle` throws - 
        // promises should probably catch and propagate exceptions
        m_chain.clear([] (auto p) { p->handle(); delete p; });
        m_futures.each([] (auto f) { f->notify(wait_result_t::Success); });
    } else if (m_abandoned) {
        m_chain.clear([] (auto p) { p->handle(); delete p; });
        m_futures.each([] (auto f) { f->notify(wait_result_t::ObjectLost); });
    }
}

void promise_data_t<void>::reassign_futures(future_t<void> *real_future) {
    promise_data_t<void> *other_data = real_future->m_data;

    m_chain.clear([&] (auto p) { other_data->m_chain.push_back(p); });
    m_futures.clear([&] (auto f) {
            f->m_data = other_data;
            other_data->m_futures.push_back(f);
        });
    other_data->notify_all();
}

// Returns true if it is safe to delete this promise_data_t
bool promise_data_t<void>::remove_future(future_t<void> *f) {
    m_futures.remove(f);
    return m_abandoned && m_futures.empty();
}

// Returns true if it is safe to delete this promise_data_t
bool promise_data_t<void>::abandon() {
    GUARANTEE(!m_abandoned);
    m_abandoned = true;
    notify_all();
    return m_futures.empty();
}

promise_t<void>::promise_t() : m_data(new promise_data_t<void>()) { }
promise_t<void>::promise_t(promise_t &&other) :
        m_data(other.m_data) {
    other.m_data = nullptr;
}

promise_t<void>::~promise_t() {
    if (m_data != nullptr && m_data->abandon()) {
        delete m_data;
    }
}

bool promise_t<void>::fulfilled() const {
    GUARANTEE(m_data != nullptr);
    return m_data->has();
}

void promise_t<void>::fulfill() {
    GUARANTEE(m_data != nullptr);
    m_data->assign();
}

future_t<void> promise_t<void>::get_future() {
    GUARANTEE(m_data != nullptr);
    return m_data->add_future();
}

} // namespace indecorous

