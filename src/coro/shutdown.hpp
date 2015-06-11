#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstdint>

namespace indecorous {

class message_hub_t;

class shutdown_t {
public:
    shutdown_t();
    void shutdown(message_hub_t *hub);
    void update(int64_t active_delta);
    void reset(uint64_t initial_count);
private:
    std::atomic<uint64_t> m_active_count;
};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
