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
const T &future_t<T>::ref() {
    wait();
    return m_data->get();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_copy_constructible<U>::value, T>::type
future_t<T>::copy() const {
    wait();
    return m_data->get();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, T>::type
future_t<T>::release() {
    wait();
    return m_data->release();
}

template <typename T, typename Callable, typename Res>
struct fulfillment_t {
    template <typename U>
    static future_t<void> run(Callable cb, U &&item) {
        promise_t<void> p;
        p.fulfill(cb(std::forward<U>(item)));
        return p.get_future();
    }
};

template <typename T, typename Callable>
struct fulfillment_t<T, Callable, void> {
    template <typename U>
    static future_t<void> run(Callable cb, U &&item) {
        promise_t<void> p;
        cb(std::forward<U>(item));
        p.fulfill();
        return p.get_future();
    }
};

template <typename T>
template <typename Callable, typename Res>
future_t<Res> future_t<T>::then(Callable cb) {
    if (m_data->has()) {
        return fulfillment_t<T, Callable, Res>::run(std::move(cb), m_data->cref());
    } else if (m_data->abandoned() || m_data->released()) {
        return promise_t<Res>().get_future(); // Return an abandoned future - can never be fulfilled
    }

    return m_data->template add_chain<Callable, Res>(std::move(cb), false);
}

template <typename T>
template <typename Callable, typename Res>
typename std::enable_if<std::is_move_constructible<T>::value, future_t<Res> >::type
future_t<T>::then_release(Callable cb) {
    if (m_data->has()) {
        return fulfillment_t<T, Callable, Res>::run(std::move(cb), m_data->release());
    } else if (m_data->abandoned() || m_data->released()) {
        return promise_t<Res>().get_future(); // Return an abandoned future - can never be fulfilled
    }

    return m_data->template add_chain<Callable, Res>(std::move(cb), true);
}

template <typename T>
future_t<T>::future_t(promise_data_t<T> *data) : m_data(data) { }

template <typename T>
void future_t<T>::add_wait(wait_callback_t *cb) {
    GUARANTEE(m_data != nullptr);
    if (m_data->has()) {
        cb->wait_done(wait_result_t::Success);
    } else if (m_data->abandoned() || m_data->released()) {
        cb->wait_done(wait_result_t::ObjectLost);
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
promise_data_t<T>::promise_data_t() :
    m_state(state_t::unfulfilled),
    m_abandoned(false) { }

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
bool promise_data_t<T>::abandoned() const {
    return m_abandoned;
}

template <typename T>
void promise_data_t<T>::assign(T &&value) {
    GUARANTEE(m_state == state_t::unfulfilled);
    new (&m_value) T(std::move(value));
    m_state = state_t::fulfilled;
    notify_all();
}

template <typename T>
void promise_data_t<T>::assign(const T &value) {
    GUARANTEE(m_state == state_t::unfulfilled);
    new (&m_value) T(value);
    m_state = state_t::fulfilled;
    notify_all();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, void>::type
promise_data_t<T>::notify_all() {
    const T &ref = m_value;
    m_chain.clear([&] (promise_chain_t *c) { c->handle(ref); delete c; });
    m_move_chain.clear([&] (promise_chain_t *c) {
            if (this->has()) c->handle_move(this->release());
            delete c;
        });
    m_futures.each([&] (future_t<T> *f) { f->notify(wait_result_t::Success); });
}

template <typename T>
template <typename U>
typename std::enable_if<!std::is_move_constructible<U>::value, void>::type
promise_data_t<T>::notify_all() {
    GUARANTEE(m_move_chain.empty());
    const T &ref = m_value;
    m_chain.clear([&] (promise_chain_t *c) { c->handle(ref); delete c; });
    m_futures.each([&] (future_t<T> *f) { f->notify(wait_result_t::Success); });
}

template <typename T>
T &promise_data_t<T>::ref() {
    GUARANTEE(m_state == state_t::fulfilled);
    return m_value;
}

template <typename T>
const T &promise_data_t<T>::cref() const {
    GUARANTEE(m_state == state_t::fulfilled);
    return m_value;
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, T>::type
promise_data_t<T>::release() {
    if (m_state == state_t::released) {
        throw wait_object_lost_exc_t();
    }
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
    GUARANTEE(!m_abandoned);
    m_abandoned = true;
    if (m_state == state_t::unfulfilled) {
        m_futures.each([&] (future_t<T> *f) {
                f->m_data = nullptr;
                f->notify(wait_result_t::ObjectLost);
            });
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
template <typename U>
typename std::enable_if<std::is_copy_constructible<U>::value, void>::type
promise_t<T>::fulfill(const T &value) {
    GUARANTEE(m_data != nullptr);
    m_data->assign(value);
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, void>::type
promise_t<T>::fulfill(T &&value) {
    GUARANTEE(m_data != nullptr);
    m_data->assign(std::move(value));
}

template <typename T>
future_t<T> promise_t<T>::get_future() {
    GUARANTEE(m_data != nullptr);
    return m_data->add_future();
}

template <typename T>
promise_data_t<T>::promise_chain_t::promise_chain_t() { }

template <typename T>
promise_data_t<T>::promise_chain_t::~promise_chain_t() { }

promise_data_t<void>::promise_chain_t::promise_chain_t() { }

promise_data_t<void>::promise_chain_t::~promise_chain_t() { }

class no_move_t { };
class no_copy_t { };

// Default promise_chain_t implementation - supports move and copy
template <typename T, typename Callable, typename Res, typename Enable = void>
class promise_chain_impl_t : public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &value) { m_promise.fulfill(m_callback(value)); }
    void handle_move(T value) { m_promise.fulfill(m_callback(std::move(value))); }
    future_t<Res> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Res> m_promise;
};

template <typename T, typename Callable, typename Res>
class promise_chain_impl_t<T, Callable, Res, typename std::enable_if<!std::is_move_constructible<T>::value, no_move_t>::type> :
        public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &value) { m_promise.fulfill(m_callback(value)); }
    void handle_move(T) { GUARANTEE(false); }
    future_t<Res> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Res> m_promise;
};

template <typename T, typename Callable, typename Res>
class promise_chain_impl_t<T, Callable, Res, typename std::enable_if<!std::is_copy_constructible<T>::value, no_copy_t>::type> :
        public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &) { GUARANTEE(false); }
    void handle_move(T value) { m_promise.fulfill(m_callback(std::move(value))); }
    future_t<Res> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Res> m_promise;
};

template <typename T, typename Callable, typename Enable>
class promise_chain_impl_t<T, Callable, void, Enable> : public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &value) { m_callback(value); m_promise.fulfill(); }
    void handle_move(T value) { m_callback(std::move(value)); m_promise.fulfill(); }
    future_t<void> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<void> m_promise;
};

template <typename T, typename Callable>
class promise_chain_impl_t<T, Callable, void, typename std::enable_if<!std::is_move_constructible<T>::value, no_move_t>::type> :
        public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &value) { m_callback(value); m_promise.fulfill(); }
    void handle_move(T) { GUARANTEE(false); }
    future_t<void> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<void> m_promise;
};

template <typename T, typename Callable>
class promise_chain_impl_t<T, Callable, void, typename std::enable_if<!std::is_copy_constructible<T>::value, no_copy_t>::type> :
        public promise_data_t<T>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle(const T &) { GUARANTEE(false); }
    void handle_move(T value) { m_callback(std::move(value)); m_promise.fulfill(); }
    future_t<void> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<void> m_promise;
};

// Used by promise_t<void>
template <typename Callable>
class promise_chain_impl_t<void, Callable, void, void> : public promise_data_t<void>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle() { m_callback(); m_promise.fulfill(); }
    future_t<void> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<void> m_promise;
};

template <typename Callable, typename Res>
class promise_chain_impl_t<void, Callable, Res, void> : public promise_data_t<void>::promise_chain_t {
public:
    promise_chain_impl_t(Callable cb) : m_callback(std::move(cb)) { }
    void handle() { m_promise.fulfill(m_callback()); }
    future_t<Res> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Res> m_promise;
};

template <typename T>
template <typename Callable, typename Res>
future_t<Res> promise_data_t<T>::add_chain(Callable cb, bool move_chain) {
    auto *chain = new promise_chain_impl_t<T, Callable, Res>(std::move(cb));
    if (move_chain) {
        m_move_chain.push_back(chain);
    } else {
        m_chain.push_back(chain);
    }
    return chain->get_future();
}

} // namespace indecorous

#endif // SYNC_PROMISE_TCC_
