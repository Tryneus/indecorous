#ifndef SYNC_PROMISE_HPP_
#define SYNC_PROMISE_HPP_

#include <memory>

#include "common.hpp"
#include "errors.hpp"
#include "containers/intrusive.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

template <typename T> class promise_data_t;

template <typename T>
class future_t : public intrusive_node_t<future_t<T> >, public wait_object_t {
public:
    future_t(future_t<T> &&other) :
            intrusive_node_t<future_t<T> >(std::move(other)),
            m_data(other.m_data),
            m_waiters(std::move(other.m_waiters)) {
        other.m_data = nullptr;
    }
    ~future_t() {
        if (m_data->remove_future(this)) {
            delete m_data;
        }
    }

    bool valid() {
        return (m_data != nullptr && !m_data->released());
    }

    T get_copy() {
        wait();
        return m_data->get();
    }

    T release() {
        wait();
        return m_data->release();
    }

    const T &get_ref() {
        wait();
        return m_data->get();
    }

private:
    friend class promise_data_t<T>;
    explicit future_t(promise_data_t<T> *data) : m_data(data) { }

    void add_wait(wait_callback_t *cb) {
        assert(m_data != nullptr);
        if (m_data->released()) {
            cb->wait_done(wait_result_t::ObjectLost);
        } else if (m_data->has()) {
            cb->wait_done(wait_result_t::Success);
        } else {
            m_waiters.push_back(cb);
        }
    }
    void remove_wait(wait_callback_t *cb) {
        m_waiters.remove(cb);
    }

    void notify(wait_result_t result) {
        while (!m_waiters.empty()) {
            m_waiters.pop_front()->wait_done(result);
        }
    }

    promise_data_t<T> *m_data;
    intrusive_list_t<wait_callback_t> m_waiters;
};

// Only to be instantiated by a promise_t
template <typename T>
class promise_data_t {
public:
    promise_data_t() : m_state(state_t::unfulfilled), m_abandoned(false) { }
    ~promise_data_t() {
        if (m_state != state_t::unfulfilled) {
            reinterpret_cast<T *>(m_buffer)->~T();
        }
    }

    bool has() const {
        return m_state == state_t::fulfilled;
    }

    bool released() const {
        return m_state == state_t::released;
    }

    void assign(T &&value) {
        assert(m_state == state_t::unfulfilled);
        new (m_buffer) T(std::move(value));
        m_state = state_t::fulfilled;

        for (future_t<T> *f = m_futures.front(); f != nullptr; f = m_futures.next(f)) {
            f->notify(wait_result_t::Success);
        }
    }

    T &get() {
        assert(m_state == state_t::fulfilled);
        return *reinterpret_cast<T *>(m_buffer);
    }

    T release() {
        T &value = get();
        m_state = state_t::released;
        return std::move(value);
    }

    future_t<T> add_future() {
        future_t<T> res(this);
        m_futures.push_back(&res);
        return res;
    }

    // Returns true if it is safe to delete this promise_data_t
    bool remove_future(future_t<T> *f) {
        m_futures.remove(f);
        return m_abandoned && m_futures.empty();
    }

    // Returns true if it is safe to delete this promise_data_t
    bool abandon() {
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

private:
    enum class state_t {
        unfulfilled,
        fulfilled,
        released,
    } m_state;

    bool m_abandoned;
    intrusive_list_t<future_t<T> > m_futures;
    alignas(T) char m_buffer[sizeof(T)];
};

template <typename T>
class promise_t {
public:
    promise_t() : m_data(new promise_data_t<T>()) { }
    promise_t(promise_t &&other) : m_data(other.m_data) { other.m_data = nullptr; }
    ~promise_t() {
        if (m_data != nullptr && m_data->abandon()) {
            delete m_data;
        }
    }

    void fulfill(T &&value) {
        assert(m_data != nullptr);
        m_data->assign(std::move(value));
    }

    future_t<T> get_future() {
        assert(m_data != nullptr);
        return m_data->add_future();
    }

private:
    promise_data_t<T> *m_data;
};

// Specialization of these classes for void
template <> class promise_data_t<void>;

template <>
class future_t<void> : public intrusive_node_t<future_t<void> >, public wait_object_t {
public:
    future_t(future_t<void> &&other);
    ~future_t();

    bool valid();

    void wait();

private:
    friend class promise_data_t<void>;
    explicit future_t(promise_data_t<void> *data);

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    void notify(wait_result_t result);

    promise_data_t<void> *m_data;
    intrusive_list_t<wait_callback_t> m_waiters;
};

template <> class promise_data_t<void> {
public:
    promise_data_t();
    ~promise_data_t();

    bool has() const;
    bool released() const;

    void assign();
    future_t<void> add_future();

    bool remove_future(future_t<void> *f);
    bool abandon();

private:
    bool m_fulfilled;
    bool m_abandoned;
    intrusive_list_t<future_t<void> > m_futures;
};

template <> class promise_t<void> {
public:
    promise_t();
    promise_t(promise_t &&other);
    ~promise_t();

    void fulfill();
    future_t<void> get_future();

private:
    promise_data_t<void> *m_data;
};

} // namespace indecorous

#endif // CORO_CORO_PROMISE_HPP_
