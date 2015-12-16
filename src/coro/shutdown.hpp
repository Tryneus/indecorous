#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace indecorous {

class target_t;

class shutdown_t {
public:
    shutdown_t(std::vector<target_t *> targets);

    void reset(size_t initial_tasks);
    void begin_shutdown();
    void update(int64_t active_delta);
private:
    const std::vector<target_t *> m_targets;
    std::atomic<bool> m_finish_sent;
    std::atomic<uint64_t> m_active_count;
};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
