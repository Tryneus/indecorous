#ifndef SYNC_PROMISE_TCC_
#define SYNC_PROMISE_TCC_

namespace indecorous {

template <typename T>
future_t<T>::future_t(future_t<T> &&other) :
        intrusive_node_t<future_t<T> >(std::move(other)),
        m_data(other.m_data),
        m_waiters(std::move(other.m_waiters)) {
    other.m_data = nullptr;
}

template <typename T>
future_t<T>::~future_t() {
    if (m_data->remove_future(this)) {
        delete m_data;
    }
}

template <typename T>
bool future_t<T>::valid() const {
    return (m_data != nullptr && !m_data->released());
}

template <typename T>
T future_t<T>::copy() const {
    wait();
    return m_data->get();
}

template <typename T>
T future_t<T>::release() {
    wait();
    return m_data->release();
}

template <typename T>
const T &future_t<T>::get_ref() {
    wait();
    return m_data->get();
}

template <typename T>
future_t<T>::future_t(promise_data_t<T> *data) : m_data(data) { }

template <typename T>
void future_t<T>::add_wait(wait_callback_t *cb) {
    assert(m_data != nullptr);
    if (m_data->released()) {
        cb->wait_done(wait_result_t::ObjectLost);
    } else if (m_data->has()) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

template <typename T>
void future_t<T>::remove_wait(wait_callback_t *cb) {
    m_waiters.remove(cb);
}

template <typename T>
void future_t<T>::notify(wait_result_t result) {
    m_waiters.clear([&] (wait_callback_t *cb) { cb->wait_done(result); });
}

// Only to be instantiated by a promise_t
template <typename T>
promise_data_t<T>::promise_data_t() : m_state(state_t::unfulfilled), m_abandoned(false) { }

template <typename T>
promise_data_t<T>::~promise_data_t() {
    if (m_state != state_t::unfulfilled) {
        m_value.~T();
    }
}

template <typename T>
bool promise_data_t<T>::has() const {
    return m_state == state_t::fulfilled;
}

template <typename T>
bool promise_data_t<T>::released() const {
    return m_state == state_t::released;
}

template <typename T>
void promise_data_t<T>::assign(T &&value) {
    assert(m_state == state_t::unfulfilled);
    new (&m_value) T(std::move(value));
    m_state = state_t::fulfilled;

    for (future_t<T> *f = m_futures.front(); f != nullptr; f = m_futures.next(f)) {
        f->notify(wait_result_t::Success);
    }
}

template <typename T>
T &promise_data_t<T>::get() {
    assert(m_state == state_t::fulfilled);
    return m_value;
}

template <typename T>
const T &promise_data_t<T>::get() const {
    assert(m_state == state_t::fulfilled);
    return m_value;
}

template <typename T>
T promise_data_t<T>::release() {
    m_state = state_t::released;
    return std::move(m_value);
}

template <typename T>
future_t<T> promise_data_t<T>::add_future() {
    future_t<T> res(this);
    m_futures.push_back(&res);
    return res;
}

// Returns true if it is safe to delete this promise_data_t
template <typename T>
bool promise_data_t<T>::remove_future(future_t<T> *f) {
    m_futures.remove(f);
    return m_abandoned && m_futures.empty();
}

// Returns true if it is safe to delete this promise_data_t
template <typename T>
bool promise_data_t<T>::abandon() {
    assert(!m_abandoned);
    m_abandoned = true;
    if (m_state == state_t::unfulfilled) {
        for (future_t<T> *f = m_futures.front(); f != nullptr; f = m_futures.next(f)) {
            f->m_data = nullptr;
            f->notify(wait_result_t::ObjectLost);
        }
    }
    return m_futures.empty();
}

template <typename T>
promise_t<T>::promise_t() : m_data(new promise_data_t<T>()) { }

template <typename T>
promise_t<T>::promise_t(promise_t &&other) : m_data(other.m_data) {
    other.m_data = nullptr;
}

template <typename T>
promise_t<T>::~promise_t() {
    if (m_data != nullptr && m_data->abandon()) {
        delete m_data;
    }
}

template <typename T>
void promise_t<T>::fulfill(T &&value) {
    assert(m_data != nullptr);
    m_data->assign(std::move(value));
}

template <typename T>
future_t<T> promise_t<T>::get_future() {
    assert(m_data != nullptr);
    return m_data->add_future();
}

} // namespace indecorous

#endif // SYNC_PROMISE_TCC_
