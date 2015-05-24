#include "coro/barrier.hpp"

namespace indecorous {

thread_barrier_t::thread_barrier_t(size_t total) :
    m_total(total), m_check_in(0), m_check_out(0) { }

void thread_barrier_t::wait() {
    std::unique_lock<std::mutex> lock(m_lock);

    // Reset a previously-used barrier (after all threads have checked out)
    if (m_check_out == m_total) {
        m_check_out = 0;
        m_check_in -= m_total;
    }

    if (++m_check_in == m_total) {
        m_cond.notify_all();
    } else {
        m_cond.wait(lock, [this] { return m_check_in == m_total; });
    }

    ++m_check_out;
}

} // namespace indecorous
