#ifndef SYNC_PROMISE_TCC_
#define SYNC_PROMISE_TCC_

#include <functional>

namespace indecorous {

// Internal type used by promise_t and future_t
template <typename T>
class promise_data_t {
public:
    promise_data_t();
    ~promise_data_t();

    bool has() const;
    bool released() const;
    bool abandoned() const;

    void assign(T &&value); // Move constructor must be available
    void assign(const T &value); // Copy constructor must be available

    const T &cref() const;

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, T>::type
    release();

    future_t<T> add_future();

    void reassign_futures(future_t<T> *real_future);

    // These return true if it is safe to delete this promise_data_t
    bool remove_future(future_t<T> *f);
    bool abandon();

    template <typename Callable, typename Res, typename Reduced>
    future_t<Reduced> add_ref_chain(Callable cb);
    template <typename Callable, typename Res, typename Reduced>
    future_t<Reduced> add_move_chain(Callable cb);

    class promise_chain_ref_t : public intrusive_node_t<promise_chain_ref_t> {
    public:
        promise_chain_ref_t() { }
        virtual ~promise_chain_ref_t() { }
        virtual void handle(const T &value) = 0;
    };

    class promise_chain_move_t : public intrusive_node_t<promise_chain_move_t> {
    public:
        promise_chain_move_t() { }
        virtual ~promise_chain_move_t() { }
        virtual void handle(T &&value) = 0;
    };

private:
    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    notify_all();

    template <typename U = T>
    typename std::enable_if<!std::is_move_constructible<U>::value, void>::type
    notify_all();

    enum class state_t {
        unfulfilled,
        fulfilled,
        released,
    };

    state_t m_state;
    bool m_abandoned;
    intrusive_list_t<future_t<T> > m_futures;
    intrusive_list_t<promise_chain_ref_t> m_ref_chain;
    intrusive_list_t<promise_chain_move_t> m_move_chain;

    union {
        T m_value;
        char m_buffer[sizeof(T)];
    };
};

// Specialization of internal type for void promises
template <> class promise_data_t<void> {
public:
    promise_data_t();
    ~promise_data_t();

    bool has() const;
    bool abandoned() const;

    void assign();
    future_t<void> add_future();

    void reassign_futures(future_t<void> *real_future);

    bool remove_future(future_t<void> *f);
    bool abandon();

    template <typename Callable, typename Res, typename Reduced>
    future_t<Reduced> add_chain(Callable cb);

    class promise_chain_t : public intrusive_node_t<promise_chain_t> {
    public:
        promise_chain_t() { }
        virtual ~promise_chain_t() { }
        virtual void handle() = 0;
    };

private:
    void notify_all();

    bool m_fulfilled;
    bool m_abandoned;
    intrusive_list_t<future_t<void> > m_futures;
    intrusive_list_t<promise_chain_t> m_chain;
};

template <typename T>
future_t<T>::future_t(future_t<T> &&other) :
        intrusive_node_t<future_t<T> >(std::move(other)),
        m_data(other.m_data),
        m_waiters(std::move(other.m_waiters)) {
    other.m_data = nullptr;
    m_waiters.each([this] (auto cb) { cb->object_moved(this); });
}

template <typename T>
future_t<T>::~future_t() {
    if (m_data != nullptr && m_data->remove_future(this)) {
        delete m_data;
    }
}

template <typename T>
bool future_t<T>::has() const {
    GUARANTEE(m_data != nullptr);
    return m_data->has();
}

template <typename T>
const T &future_t<T>::get() {
    wait();
    return m_data->cref();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_copy_constructible<U>::value, T>::type
future_t<T>::copy() {
    wait();
    return m_data->cref();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, T>::type
future_t<T>::release() {
    wait();
    return m_data->release();
}

template <typename T, typename Callable, typename Res, typename Reduced>
struct fulfillment_t {
    template <typename U>
    static future_t<Reduced> run(Callable cb, U &&item) {
        promise_t<Reduced> p;
        p.fulfill(cb(std::forward<U>(item)));
        return p.get_future();
    }
};

template <typename T, typename Callable>
struct fulfillment_t<T, Callable, void, void> {
    template <typename U>
    static future_t<void> run(Callable cb, U &&item) {
        promise_t<void> p;
        cb(std::forward<U>(item));
        p.fulfill();
        return p.get_future();
    }
};

template <typename T, typename Callable, typename Reduced>
struct fulfillment_t<T, Callable, future_t<Reduced>, Reduced> {
    template <typename U>
    static future_t<Reduced> run(Callable cb, U &&item) {
        return cb(std::forward<U>(item));
    }
};

template <typename Callable, typename Res, typename Reduced>
struct fulfillment_t<void, Callable, Res, Reduced> {
    static future_t<Reduced> run(Callable cb) {
        promise_t<Reduced> p;
        p.fulfill(cb());
        return p.get_future();
    }
};

template <typename Callable>
struct fulfillment_t<void, Callable, void, void> {
    static future_t<void> run(Callable cb) {
        promise_t<void> p;
        cb();
        p.fulfill();
        return p.get_future();
    }
};

template <typename Callable, typename Reduced>
struct fulfillment_t<void, Callable, future_t<Reduced>, Reduced> {
    static future_t<Reduced> run(Callable cb) {
        return cb();
    }
};

template <typename T>
template <typename Callable, typename Res, typename Reduced>
future_t<Reduced> future_t<T>::then(Callable cb) {
    if (m_data->has()) {
        return fulfillment_t<T, Callable, Res, Reduced>::run(std::move(cb), m_data->cref());
    } else if (m_data->abandoned() || m_data->released()) {
        return promise_t<Reduced>().get_future(); // Return an abandoned future - can never be fulfilled
    }

    return m_data->template add_ref_chain<Callable, Res, Reduced>(std::move(cb));
}

template <typename T>
template <typename Callable, typename Res, typename Reduced>
typename std::enable_if<std::is_move_constructible<T>::value, future_t<Reduced> >::type
future_t<T>::then_release(Callable cb) {
    if (m_data->has()) {
        return fulfillment_t<T, Callable, Res, Reduced>::run(std::move(cb), m_data->release());
    } else if (m_data->abandoned() || m_data->released()) {
        return promise_t<Reduced>().get_future(); // Return an abandoned future - can never be fulfilled
    }

    return m_data->template add_move_chain<Callable, Res, Reduced>(std::move(cb));
}

template <typename Callable, typename Res, typename Reduced>
future_t<Reduced> future_t<void>::then(Callable cb) {
    if (m_data->has()) {
        return fulfillment_t<void, Callable, Res, Reduced>::run(std::move(cb));
    } else if (m_data->abandoned()) {
        return promise_t<Reduced>().get_future(); // Return an abandoned future - can never be fulfilled
    }

    return m_data->template add_chain<Callable, Res, Reduced>(std::move(cb));
}

template <typename T>
future_t<T>::future_t(promise_data_t<T> *data) :
    m_data(data),
    m_waiters() { }

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
    m_waiters.clear([result] (auto cb) { cb->wait_done(result); });
}

// Only to be instantiated by a promise_t
template <typename T>
promise_data_t<T>::promise_data_t() :
    m_state(state_t::unfulfilled),
    m_abandoned(false),
    m_futures(),
    m_ref_chain(),
    m_move_chain() { }

template <typename T>
promise_data_t<T>::~promise_data_t() {
    if (m_state != state_t::unfulfilled) {
        m_value.~T();
    }
    m_ref_chain.clear([] (auto p) { delete p; });
    m_move_chain.clear([] (auto p) { delete p; });
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
void promise_data_t<T>::reassign_futures(future_t<T> *real_future) {
    promise_data_t<T> *other_data = real_future->m_data;

    m_ref_chain.clear([&] (auto p) { other_data->m_ref_chain.push_back(p); });
    m_move_chain.clear([&] (auto p) { other_data->m_move_chain.push_back(p); });
    m_futures.clear([&] (auto f) {
            f->m_data = other_data;
            other_data->m_futures.push_back(f);
        });
    other_data->notify_all();
}

template <typename T>
template <typename U>
typename std::enable_if<std::is_move_constructible<U>::value, void>::type
promise_data_t<T>::notify_all() {
    if (m_state == state_t::fulfilled) {
        const T &local_ref = m_value;
        m_ref_chain.clear([&] (auto p) { p->handle(local_ref); delete p; });
        m_move_chain.clear([&] (auto p) {
                if (this->has()) p->handle(this->release());
                delete p;
            });
        m_futures.each([] (auto f) { f->notify(wait_result_t::Success); });
    } else if (m_state == state_t::released || m_abandoned) {
        m_futures.each([] (auto f) { f->notify(wait_result_t::ObjectLost); });
    }
}

template <typename T>
template <typename U>
typename std::enable_if<!std::is_move_constructible<U>::value, void>::type
promise_data_t<T>::notify_all() {
    if (m_state == state_t::fulfilled) {
        GUARANTEE(m_move_chain.empty());
        m_ref_chain.clear([&] (auto p) { p->handle(m_value); delete p; });
        m_futures.each([&] (auto f) { f->notify(wait_result_t::Success); });
    } else if (m_state == state_t::released || m_abandoned) {
        m_futures.each([] (auto f) { f->notify(wait_result_t::ObjectLost); });
    }
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
    notify_all();
    return m_futures.empty();
}

template <typename T>
promise_t<T>::promise_t() : 
    m_data(new promise_data_t<T>()) { }

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
bool promise_t<T>::fulfilled() const {
    GUARANTEE(m_data != nullptr);
    return m_data->has();
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

// T -> Reduced
template <typename T, typename Callable, typename Res, typename Reduced>
struct chain_fulfillment_ref_t {
    static void run(Callable &cb, const T &value, promise_t<Res> *out) {
        out->fulfill(cb(value));
    }
};

// T -> void
template <typename T, typename Callable>
struct chain_fulfillment_ref_t<T, Callable, void, void> {
    static void run(Callable &cb, const T &value, promise_t<void> *out) {
        cb(value);
        out->fulfill();
    }
};

// T -> future_t<Reduced>
template <typename T, typename Callable, typename Reduced>
struct chain_fulfillment_ref_t<T, Callable, future_t<Reduced>, Reduced> {
    static void run(Callable &cb, const T &value, promise_t<Reduced> *out) {
        future_t<Reduced> real_future = cb(value);
        out->m_data->reassign_futures(&real_future);
    }
};

// T -> Reduced
template <typename T, typename Callable, typename Res, typename Reduced>
struct chain_fulfillment_move_t {
    template <typename U = T>
    static typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    run(Callable &cb, T value, promise_t<Reduced> *out) {
        out->fulfill(cb(std::move(value)));
    }
};

// T -> void
template <typename T, typename Callable>
struct chain_fulfillment_move_t<T, Callable, void, void> {
    template <typename U = T>
    static typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    run(Callable &cb, T value, promise_t<void> *out) {
        cb(std::move(value));
        out->fulfill();
    }
};

// T -> future_t<Reduced>
template <typename T, typename Callable, typename Reduced>
struct chain_fulfillment_move_t<T, Callable, future_t<Reduced>, Reduced> {
    template <typename U = T>
    static typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    run(Callable &cb, T value, promise_t<Reduced> *out) {
        future_t<Reduced> real_future = cb(std::move(value));
        out->m_data->reassign_futures(&real_future);
    }
};

template <typename T, typename Callable, typename Res, typename Reduced>
class promise_chain_ref_impl_t : public promise_data_t<T>::promise_chain_ref_t {
public:
    promise_chain_ref_impl_t(Callable cb) :
        m_callback(std::move(cb)), m_promise() { }
    void handle(const T &value) {
        chain_fulfillment_ref_t<T, Callable, Res, Reduced>::run(m_callback, value, &m_promise);
    }
    future_t<Reduced> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Reduced> m_promise;
};

template <typename T, typename Callable, typename Res, typename Reduced>
class promise_chain_move_impl_t : public promise_data_t<T>::promise_chain_move_t {
public:
    promise_chain_move_impl_t(Callable cb) :
        m_callback(std::move(cb)), m_promise() { }
    void handle(T &&value) {
        chain_fulfillment_move_t<T, Callable, Res, Reduced>::run(m_callback, std::move(value), &m_promise);
    }
    future_t<Reduced> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Reduced> m_promise;
};

// chain void -> Reduced
template <typename Callable, typename Res, typename Reduced>
class promise_chain_void_impl_t : public promise_data_t<void>::promise_chain_t {
public:
    promise_chain_void_impl_t(Callable cb) :
        m_callback(std::move(cb)), m_promise() { }
    void handle() { m_promise.fulfill(m_callback()); }
    future_t<Reduced> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Reduced> m_promise;
};

// chain void -> void
template <typename Callable>
class promise_chain_void_impl_t<Callable, void, void> :
        public promise_data_t<void>::promise_chain_t {
public:
    promise_chain_void_impl_t(Callable cb) :
        m_callback(std::move(cb)), m_promise() { }
    void handle() { m_callback(); m_promise.fulfill(); }
    future_t<void> get_future() { return m_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<void> m_promise;
};

// chain void -> future_t<Reduced>
template <typename Callable, typename Reduced>
class promise_chain_void_impl_t<Callable, future_t<Reduced>, Reduced> :
        public promise_data_t<void>::promise_chain_t {
public:
    promise_chain_void_impl_t(Callable cb) :
        m_callback(std::move(cb)), m_dummy_promise() { }
    void handle() {
        future_t<Reduced> real_future = m_callback();
        m_dummy_promise.m_data->reassign_futures(&real_future);
    }
    future_t<Reduced> get_future() { return m_dummy_promise.get_future(); }
private:
    Callable m_callback;
    promise_t<Reduced> m_dummy_promise;
};

template <typename T>
template <typename Callable, typename Res, typename Reduced>
future_t<Reduced> promise_data_t<T>::add_ref_chain(Callable cb) {
    auto *chain = new promise_chain_ref_impl_t<T, Callable, Res, Reduced>(std::move(cb));
    m_ref_chain.push_back(chain);
    return chain->get_future();
}

template <typename T>
template <typename Callable, typename Res, typename Reduced>
future_t<Reduced> promise_data_t<T>::add_move_chain(Callable cb) {
    auto *chain = new promise_chain_move_impl_t<T, Callable, Res, Reduced>(std::move(cb));
    m_move_chain.push_back(chain);
    return chain->get_future();
}

template <typename Callable, typename Res, typename Reduced>
future_t<Reduced> promise_data_t<void>::add_chain(Callable cb) {
    auto *chain = new promise_chain_void_impl_t<Callable, Res, Reduced>(std::move(cb));
    m_chain.push_back(chain);
    return chain->get_future();
}

} // namespace indecorous

#endif // SYNC_PROMISE_TCC_
