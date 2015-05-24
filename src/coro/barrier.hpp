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
    const size_t m_total;
    size_t m_check_in;
    size_t m_check_out;
    std::mutex m_lock;
    std::condition_variable m_cond;
};

} // namespace indecorous

#endif // CORO_BARRIER_HPP_
