#ifndef CROSS_THREAD_FLIP_BUFFER_HPP_
#define CROSS_THREAD_FLIP_BUFFER_HPP_

#include <vector>

#include "cross_thread/home_threaded.hpp"
#include "rpc/hub.hpp"
#include "sync/drainer.hpp"
#include "sync/mutex.hpp"
#include "sync/swap.hpp"

namespace indecorous {

class flip_buffer_base_t : public home_threaded_t {
public:
    flip_buffer_base_t();
    virtual ~flip_buffer_base_t();

    // TODO: privatize this
    void flip_internal();
protected:
    void flip();

    enum class buffer_id_t { BUFFER_A = 0, BUFFER_B = 1 };
    buffer_id_t current_buffer_id() const;

    std::vector<buffer_id_t> m_current_buffers;
};

// Note that the type T must be copy-constructible, but it does not need to be
// move-constructible or default-constructible.
// The flip_buffer_t can be safely read from any thread, but can only be written to
// on the same thread which created it.
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
        cb((current_buffer_id() == buffer_id_t::BUFFER_A) ? m_buffer_a : m_buffer_b);
    }

    template <typename Callable>
    void apply_write(Callable &&cb) {
        assert_thread();
        drainer_lock_t drainer_lock = m_drainer.lock();
        mutex_acq_t mutex_acq = m_mutex.start_acq();
        mutex_acq.wait();

        T *old_buffer;
        T *new_buffer;

        if (current_buffer_id() == buffer_id_t::BUFFER_A) {
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
    drainer_t m_drainer;
    mutex_t m_mutex;
    T m_buffer_a;
    T m_buffer_b;
};

} // namespace indecorous

#endif // SYNC_FLIP_BUFFER_HPP_
