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

template <typename T> class future_t;

template <typename T>
struct future_reducer_t {
    static T reduce();
};

template <typename T>
struct future_reducer_t<future_t<T> > {
    static T reduce();
};

template <typename T>
class future_t : public intrusive_node_t<future_t<T> >, public waitable_t {
public:
    future_t(future_t<T> &&other);
    ~future_t();

    bool has() const;

    template <typename U = T>
    typename std::enable_if<std::is_copy_constructible<U>::value, T>::type
    copy();

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, T>::type
    release();

    const T &get();

    // `then` provides a const reference to the value which may be used to create a copy
    template <typename Callable,
              typename Res = typename std::result_of<Callable(T)>::type,
              typename Reduced = decltype(future_reducer_t<Res>::reduce())>
    future_t<Reduced> then(Callable cb);

    // `then_release` provides a move of the value and can only be used if T is move-constructible
    template <typename Callable,
              typename Res = typename std::result_of<Callable(T)>::type,
              typename Reduced = decltype(future_reducer_t<Res>::reduce())>
    typename std::enable_if<std::is_move_constructible<T>::value, future_t<Reduced> >::type
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
class future_t<void> : public intrusive_node_t<future_t<void> >, public waitable_t {
public:
    future_t(future_t<void> &&other);
    ~future_t();

    bool has() const;

    template <typename Callable,
              typename Res = typename std::result_of<Callable()>::type,
              typename Reduced = decltype(future_reducer_t<Res>::reduce())>
    future_t<Reduced> then(Callable cb);

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

    bool fulfilled() const;

    template <typename U = T>
    typename std::enable_if<std::is_copy_constructible<U>::value, void>::type
    fulfill(const T &value);

    template <typename U = T>
    typename std::enable_if<std::is_move_constructible<U>::value, void>::type
    fulfill(T &&value);

    future_t<T> get_future();

private:
    template <typename U, typename Callable, typename Res, typename Reduced>
    friend struct chain_fulfillment_ref_t;
    template <typename U, typename Callable, typename Res, typename Reduced>
    friend struct chain_fulfillment_move_t;
    template <typename Callable, typename Res, typename Reduced>
    friend class promise_chain_void_impl_t;

    promise_data_t<T> *m_data;
};

template <> class promise_t<void> {
public:
    promise_t();
    promise_t(promise_t &&other);
    ~promise_t();

    bool fulfilled() const;
    void fulfill();
    future_t<void> get_future();

private:
    template <typename U, typename Callable, typename Res, typename Reduced>
    friend struct chain_fulfillment_ref_t;
    template <typename U, typename Callable, typename Res, typename Reduced>
    friend struct chain_fulfillment_move_t;
    template <typename Callable, typename Res, typename Reduced>
    friend class promise_chain_void_impl_t;

    promise_data_t<void> *m_data;
};

} // namespace indecorous

#include "sync/promise.tcc"

#endif // CORO_CORO_PROMISE_HPP_
