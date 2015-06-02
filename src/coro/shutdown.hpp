#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstdint>
#include <list>

namespace indecorous {

class thread_t;

class shutdown_t {
public:
    shutdown_t();
    void shutdown(std::list<thread_t> *threads);
    bool update(int64_t active_delta);
    void reset();
private:
    std::atomic<uint64_t> m_active_count;
};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
