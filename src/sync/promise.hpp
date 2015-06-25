#ifndef SYNC_PROMISE_HPP_
#define SYNC_PROMISE_HPP_

#include <memory>
#include <type_traits>

#include "common.hpp"
#include "errors.hpp"
#include "containers/intrusive.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

template <typename T> class promise_data_t;
template <> class promise_data_t<void>;

template <typename T>
class future_t : public intrusive_node_t<future_t<T> >, public wait_object_t {
public:
    future_t(future_t<T> &&other);
    ~future_t();

    bool valid() const;

    template <typename U = T>
    typename std::enable_if<std::is_copy_constructible<U>::value, T>::type
    copy() const;

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, T>::type
    release();

    const T &ref();

    // `then` provides a const reference to the value which may be used to create a copy
    template <typename Callable, typename Res = typename std::result_of<Callable(T)>::type>
    future_t<Res> then(Callable cb);

    // `then_release` provides a move of the value and can only be used if T is move-constructible
    template <typename Callable, typename Res = typename std::result_of<Callable(T)>::type>
    typename std::enable_if<std::is_move_constructible<T>::value, future_t<Res> >::type
    then_release(Callable cb);

private:
    friend class promise_data_t<T>;
    explicit future_t(promise_data_t<T> *data);

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    void notify(wait_result_t result);

    promise_data_t<T> *m_data;
    intrusive_list_t<wait_callback_t> m_waiters;
};

template <>
class future_t<void> : public intrusive_node_t<future_t<void> >, public wait_object_t {
public:
    future_t(future_t<void> &&other);
    ~future_t();

    bool valid() const;

    template <typename Callable, typename Res = typename std::result_of<Callable()>::type>
    future_t<Res> then(Callable cb);

private:
    friend class promise_data_t<void>;
    explicit future_t(promise_data_t<void> *data);

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    void notify(wait_result_t result);

    promise_data_t<void> *m_data;
    intrusive_list_t<wait_callback_t> m_waiters;
};

template <typename T>
class promise_t {
public:
    promise_t(promise_t &&other);
    promise_t();
    ~promise_t();

    template <typename U = T>
    typename std::enable_if<std::is_copy_constructible<U>::value, void>::type
    fulfill(const T &value);

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    fulfill(T &&value);

    future_t<T> get_future();

private:
    promise_data_t<T> *m_data;
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

// Only to be instantiated by a promise_t
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

    T &ref();
    const T &cref() const;

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, T>::type
    release();

    future_t<T> add_future();

    // These return true if it is safe to delete this promise_data_t
    bool remove_future(future_t<T> *f);
    bool abandon();

    template <typename Callable, typename Res>
    future_t<Res> add_chain(Callable cb, bool move_chain);

    // This class is used internally to chain promises together without using extra coroutines
    class promise_chain_t : public intrusive_node_t<promise_chain_t> {
    public:
        promise_chain_t();
        virtual ~promise_chain_t();
        virtual void handle(const T &value) = 0;
        virtual void handle_move(T value) = 0;
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
    intrusive_list_t<promise_chain_t> m_chain;
    intrusive_list_t<promise_chain_t> m_move_chain;

    union {
        T m_value;
        char m_buffer[sizeof(T)];
    };
};

template <> class promise_data_t<void> {
public:
    promise_data_t();
    ~promise_data_t();

    bool has() const;
    bool abandoned() const;

    void assign();
    future_t<void> add_future();

    bool remove_future(future_t<void> *f);
    bool abandon();

    template <typename Callable, typename Res>
    future_t<Res> add_chain(Callable cb);

    class promise_chain_t : public intrusive_node_t<promise_chain_t> {
    public:
        promise_chain_t();
        virtual ~promise_chain_t();
        virtual void handle() = 0;
    };

private:
    bool m_fulfilled;
    bool m_abandoned;
    intrusive_list_t<future_t<void> > m_futures;
    intrusive_list_t<promise_chain_t> m_chains;
};

} // namespace indecorous

#include "sync/promise.tcc"

#endif // CORO_CORO_PROMISE_HPP_
