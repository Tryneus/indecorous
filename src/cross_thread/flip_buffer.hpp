#ifndef CROSS_THREAD_FLIP_BUFFER_HPP_
#define CROSS_THREAD_FLIP_BUFFER_HPP_

#include <vector>
#include <memory>

#include "common.hpp"
#include "containers/one_per_thread.hpp"
#include "cross_thread/ct_mutex.hpp"
#include "rpc/hub.hpp"
#include "sync/drainer.hpp"
#include "sync/swap.hpp"

namespace indecorous {

class flip_buffer_base_t {
public:
    flip_buffer_base_t();
    virtual ~flip_buffer_base_t();

protected:
    friend class flip_buffer_callback_t;
    void flip_internal();
    void flip();

    virtual void on_flip() const = 0;

    enum class buffer_id_t { BUFFER_A = 0, BUFFER_B = 1 };
    one_per_thread_t<buffer_id_t> m_current_buffer;

    DISABLE_COPYING(flip_buffer_base_t);
};

// Note that the type T must be copy-constructible, but it does not need to be
// move-constructible or default-constructible.
// The flip_buffer_t can be safely written or read from any thread, although write
// performance may be impacted if there is much contention for the mutex.
template <typename T>
class flip_buffer_t : public flip_buffer_base_t {
public:
    template <typename ...Args>
    flip_buffer_t(Args &&...args) :
        flip_buffer_base_t(),
        m_buffer_a(std::forward<Args>(args)...),
        m_buffer_b(m_buffer_a) { }

    ~flip_buffer_t() { }

    template <typename Callable>
    void apply_read(Callable &&cb) const {
        assert_no_swap_t no_swap;
        cb(active_buffer());
    }

    template <typename Callable>
    void apply_write(Callable &&cb) {
        drainer_lock_t drainer_lock = m_drainer.lock();
        cross_thread_mutex_acq_t mutex_acq(&m_mutex);
        mutex_acq.wait();

        T *old_buffer;
        T *new_buffer;

        if (m_current_buffer.get() == buffer_id_t::BUFFER_A) {
            old_buffer = &m_buffer_a;
            new_buffer = &m_buffer_b;
        } else {
            old_buffer = &m_buffer_b;
            new_buffer = &m_buffer_a;
        }

        {
            assert_no_swap_t no_swap;
            cb(*new_buffer);
        }

        flip();

        {
            assert_no_swap_t no_swap;
            *old_buffer = *new_buffer;
        }
    }

private:
    T &active_buffer() {
        return (m_current_buffer.get() == buffer_id_t::BUFFER_A) ? m_buffer_a : m_buffer_b;
    }

    cross_thread_mutex_t m_mutex;
    T m_buffer_a;
    T m_buffer_b;

    drainer_t m_drainer;

    DISABLE_COPYING(flip_buffer_t);
};

} // namespace indecorous

#endif // SYNC_FLIP_BUFFER_HPP_
