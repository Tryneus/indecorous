#include "coro/barrier.hpp"

namespace indecorous {

thread_barrier_t::thread_barrier_t(size_t total) :
    m_total(total), m_side(side_t::A),
    m_count_a(0), m_count_b(0),
    m_lock(), m_cond() { }

void thread_barrier_t::wait_internal(size_t *count, size_t *reset,
                                     std::unique_lock<std::mutex> &lock) {
    if (++*count == m_total) {
        *reset = 0;
        m_cond.notify_all();
    } else {
        m_cond.wait(lock, [&] { return *count == m_total; });
    }
}

void thread_barrier_t::wait() {
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_side == side_t::A) {
        wait_internal(&m_count_a, &m_count_b, lock);
        m_side = side_t::B;
    } else {
        wait_internal(&m_count_b, &m_count_a, lock);
        m_side = side_t::A;
    }
}

} // namespace indecorous
