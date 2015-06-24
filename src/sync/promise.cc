#include "sync/promise.hpp"

namespace indecorous {

// Specialization of these classes for void
future_t<void>::future_t(future_t<void> &&other) :
        intrusive_node_t<future_t<void> >(std::move(other)),
        m_data(other.m_data),
        m_waiters(std::move(other.m_waiters)) {
    other.m_data = nullptr;
}

future_t<void>::future_t(promise_data_t<void> *data) : m_data(data) { }

future_t<void>::~future_t() {
    if (m_data->remove_future(this)) {
        delete m_data;
    }
}

bool future_t<void>::valid() const {
    return m_data != nullptr;
}

template <typename Callable, typename Res>
future_t<Res> future_t<void>::then(Callable cb) {
    return m_data->add_chain<Callable, Res>(cb);
}

template <typename Callable, typename Res>
future_t<Res> future_t<void>::then_release(Callable cb) {
    return m_data->add_chain<Callable, Res>(cb);
}

void future_t<void>::add_wait(wait_callback_t *cb) {
    assert(m_data != nullptr);
    if (m_data->has()) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}
void future_t<void>::remove_wait(wait_callback_t *cb) {
    m_waiters.remove(cb);
}

void future_t<void>::notify(wait_result_t result) {
    m_waiters.clear([&] (wait_callback_t *cb) { cb->wait_done(result); });
}

promise_data_t<void>::promise_data_t() : m_fulfilled(false), m_abandoned(false) { }

promise_data_t<void>::~promise_data_t() { }

bool promise_data_t<void>::has() const { return m_fulfilled; }
bool promise_data_t<void>::released() const { return false; }

void promise_data_t<void>::assign() {
    assert(m_fulfilled == false);
    m_fulfilled = true;
    for (future_t<void> *f = m_futures.front(); f != nullptr; f = m_futures.next(f)) {
        f->notify(wait_result_t::Success);
    }
    m_chains.each([&] (promise_chain_t *c) { c->handle(); });
}

future_t<void> promise_data_t<void>::add_future() {
    future_t<void> res(this);
    m_futures.push_back(&res);
    return res;
}

// Returns true if it is safe to delete this promise_data_t
bool promise_data_t<void>::remove_future(future_t<void> *f) {
    m_futures.remove(f);
    return m_abandoned && m_futures.empty();
}

// Returns true if it is safe to delete this promise_data_t
bool promise_data_t<void>::abandon() {
    assert(!m_abandoned);
    m_abandoned = true;
    if (m_fulfilled == false) {
        for (future_t<void> *f = m_futures.front(); f != nullptr; f = m_futures.next(f)) {
            f->m_data = nullptr;
            f->notify(wait_result_t::ObjectLost);
        }
    }
    return m_futures.empty();
}

template <typename Callable, typename Res>
future_t<Res> promise_data_t<void>::add_chain(Callable cb) {
    auto *chain = new promise_chain_impl_t<void, Callable, Res>(std::move(cb));
    m_chains.push_back(chain);
    return chain->get_future();
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

void promise_t<void>::fulfill() {
    assert(m_data != nullptr);
    m_data->assign();
}

future_t<void> promise_t<void>::get_future() {
    assert(m_data != nullptr);
    return m_data->add_future();
}

promise_data_t<void>::promise_chain_t::promise_chain_t() { }

promise_data_t<void>::promise_chain_t::~promise_chain_t() { }

} // namespace indecorous

