#ifndef SYNC_FLIP_BUFFER_HPP_
#define SYNC_FLIP_BUFFER_HPP_

#include <mutex>
#include <atomic>

#include "rpc/hub.hpp"
#include "rpc/hub_data.hpp"
#include "sync/swap.hpp"

class flip_buffer_base_t {
public:
    ~flip_buffer_base_t() { }
    virtual void flip() = 0;
};

struct flip_buffer_callback_t : public handler_t<flip_buffer_callback_t> {
    void call(flip_buffer_base_t *flip) {
        flip->flip();
    }
};

template <typename T>
class flip_buffer_t {
public:
    flip_buffer_t() : m_current_read(0) { }
    ~flip_buffer_t() { }

    template <typename Callable>
    void apply_read(Callable &&c) const {
        assert_no_swap_t no_swap;
        c(m_buffer[m_current_read.load()]);
    }

    // TODO: use a throttling pool to combine multiple writes so we don't lag behind
    template <typename Callable>
    void apply_write(Callable &&c) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t new_offset = (m_current_read.load() + 1) % 2;

        {
            assert_no_swap_t no_swap;
            c(m_buffer[new_offset]);
        }

        thread_t::self()->hub()->broadcast_local_sync<flip_buffer_callback_t>(this);
        m_current_read.store(new_offset);
    }


private:
    std::mutex m_mutex;
    std::atomic<size_t> m_current_read; // TODO: this should be thread-local or sized by thread ids
    T m_buffer[2];
};

#endif // SYNC_FLIP_BUFFER_HPP_
