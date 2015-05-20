#ifndef SYNC_PROMISE_HPP_
#define SYNC_PROMISE_HPP_

#include <memory>

#include "common.hpp"
#include "errors.hpp"
#include "containers/queue.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

template <typename T> class promise_data_t;

template <typename T>
class future_t : public intrusive_node_t<future_t<T> >, public wait_object_t {
public:
    future_t(future_t<T> &&other) :
            intrusive_node_t<T>(std::move(other)),
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

    void wait() {
        if (m_data == nullptr || m_data->released()) {
            throw wait_object_lost_exc_t(); // TODO: streamline this logic with the stuff in coro_t
        } else if (m_data->has()) {
            return;
        } else {
            m_data->m_waiters.push(coro_self());
            coro_wait();
        }
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
    future_t(promise_data_t<T> *data) : m_data(data) { }

    void addWait(wait_callback_t *cb) {
        if (m_data == nullptr || m_data->released()) {
            cb->wait_callback(wait_result_t::ObjectLost);
        } else if (m_data->has()) {
            cb->wait_callback(wait_result_t::Success);
        } else {
            m_data->m_waiters.push(cb);
        }
    }
    void removeWait(wait_callback_t *cb) {
        m_waiters.remove(cb);
    }

    void notify(wait_result_t result) {
        while (!m_waiters.empty()) {
            m_waiters.pop()->wait_callback(result);
        }
    }

    promise_data_t<T> *m_data;
    intrusive_queue_t<wait_callback_t> m_waiters;
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

    void has() const {
        return m_state == state_t::fulfilled;
    }

    void released() const {
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
        m_futures.push(&res);
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
    intrusive_queue_t<future_t<T> > m_futures;
    alignas(T) char m_buffer[sizeof(T)];
};

template <typename T>
class promise_t {
public:
    promise_t() :
        m_data(new promise_data_t<T>()) { }
    ~promise_t() {
        if (m_data->abandon()) {
            delete m_data;
        }
    }

    void fulfill(T &&value) {
        m_data->assign(std::move(value));
    }

    future_t<T> get_future() {
        return m_data->add_future();
    }

private:
    promise_data_t<T> *m_data;
};

} // namespace indecorous

#endif // CORO_CORO_PROMISE_HPP_
