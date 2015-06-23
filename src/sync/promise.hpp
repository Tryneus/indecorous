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
    future_t(future_t<T> &&other);
    ~future_t();

    bool valid() const;

    T copy() const;
    T release();
    const T &get_ref();

private:
    friend class promise_data_t<T>;
    explicit future_t(promise_data_t<T> *data);

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    void notify(wait_result_t result);

    promise_data_t<T> *m_data;
    intrusive_list_t<wait_callback_t> m_waiters;
};

// Only to be instantiated by a promise_t
template <typename T>
class promise_data_t {
public:
    promise_data_t();
    ~promise_data_t();

    bool has() const;
    bool released() const;

    void assign(T &&value);

    T &get();
    const T &get() const;
    T release();

    future_t<T> add_future();

    // These return true if it is safe to delete this promise_data_t
    bool remove_future(future_t<T> *f);
    bool abandon();

private:
    enum class state_t {
        unfulfilled,
        fulfilled,
        released,
    } m_state;

    bool m_abandoned;
    intrusive_list_t<future_t<T> > m_futures;

    union {
        T m_value;
        char m_buffer[sizeof(T)];
    };
};

template <typename T>
class promise_t {
public:
    promise_t(promise_t &&other);
    promise_t();
    ~promise_t();

    void fulfill(T &&value);
    future_t<T> get_future();

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

    bool valid() const;
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

#include "sync/promise.tcc"

#endif // CORO_CORO_PROMISE_HPP_
