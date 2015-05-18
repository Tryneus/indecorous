#ifndef SYNC_PROMISE_HPP_
#define SYNC_PROMISE_HPP_

#include <memory>

namespace indecorous {

template <typename T>
class promise_t : public wait_object_t {
public:
    promise_t() :
        m_data(new data_t()) { }
    promise_t(promise_t &&other) :
        m_data(std::move(other.m_data)) { }
    promise_t(const promise_t &other) :
        m_data(other.m_data()) { }
    ~promise_t() { }

    void fulfill(T &&value) {
        m_data->assign(std::move(value));
        while (!m_data->waiters.empty()) {
            m_data->waiters.pop()->wait_callback(wait_result_t::Success);
        }
    }

    T get_copy() {
        wait();
        return m_data->get();
    }

    T release() {
        return std::move(m_data->get());
    }

    const T &get_ref() {
        wait();
        return m_data->get();
    }

    void wait() {
        // TODO: fix this circular dependency
        /*
        if (m_data->has) {
            return;
        } else {
            m_data->m_waiters.push(coro_t::self());
            coro_t::wait();
        }
        */
    }

private:
    void addWait(wait_callback_t *cb) {
        if (m_data->has) {
            cb->wait_callback(wait_result_t::Success);
        } else {
            m_data->m_waiters.push(cb);
        }
    }
    void removeWait(wait_callback_t *cb) {
        m_data->waiters.remove(cb);
    }

    class data_t {
    public:
        data_t() { }
        ~data_t() {
            if (has) {
                reinterpret_cast<T *>(buffer)->~T();
            }
        }

        void assign(T &&value) {
            assert(!has);
            new (buffer) T(std::move(value));
            has = true;
        }

        T &get() {
            assert(has);
            return *reinterpret_cast<T *>(buffer);
        }

    private:
        bool has;
        alignas(T) char buffer[sizeof(T)];
        IntrusiveQueue<wait_callback_t> waiters;
    };
    std::shared_ptr<data_t> m_data;
};

} // namespace indecorous

#endif // CORO_CORO_PROMISE_HPP_
