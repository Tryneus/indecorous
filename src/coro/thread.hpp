#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <thread>

#include "coro/coro.hpp"
#include "coro/events.hpp"
#include "rpc/target.hpp"

namespace indecorous {

class message_hub_t;
class scheduler_t;
class thread_barrier_t;

class thread_t {
public:
    thread_t(message_hub_t *hub, thread_barrier_t *barrier);

    static thread_t *self();

    target_id_t id() const;

    dispatcher_t *dispatcher();
    events_t *events();
    local_target_t *target();

    void shutdown();

private:
    void main();

    thread_barrier_t *m_barrier;

    events_t m_events;
    dispatcher_t m_dispatch;
    local_target_t m_target;

    bool m_shutdown;
    std::thread m_thread;

    thread_local static thread_t *s_instance;
};

} // namespace indecorous

#endif // CORO_THREAD_HPP_
