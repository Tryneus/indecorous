#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstdint>
#include <cstddef>

namespace indecorous {

class message_hub_t;

class shutdown_t {
public:
    explicit shutdown_t(size_t thread_count);
    void shutdown(message_hub_t *hub);
    void update(int64_t active_delta, message_hub_t *hub);
    void reset(uint64_t initial_count);
private:
    std::atomic<bool> m_finish_sent;
    std::atomic<uint64_t> m_active_count;
    size_t m_thread_count;
};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
