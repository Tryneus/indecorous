#ifndef CORO_BARRIER_HPP_
#define CORO_BARRIER_HPP_

#include <cstddef>
#include <mutex>
#include <condition_variable>

namespace indecorous {

class thread_barrier_t {
public:
    explicit thread_barrier_t(size_t total);
    void wait();
private:
    void wait_internal(size_t *count, size_t *reset,
                       std::unique_lock<std::mutex> &lock);

    const size_t m_total;
    enum class side_t { A, B } m_side;
    size_t m_count_a;
    size_t m_count_b;
    std::mutex m_lock;
    std::condition_variable m_cond;
};

} // namespace indecorous

#endif // CORO_BARRIER_HPP_
