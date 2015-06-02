#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <atomic>
#include <thread>

#include "coro/coro.hpp"
#include "coro/events.hpp"
#include "coro/shutdown.hpp"
#include "rpc/target.hpp"

namespace indecorous {

class message_hub_t;
class scheduler_t;
class thread_barrier_t;

class thread_t {
public:
    thread_t(shutdown_t *shutdown,
             thread_barrier_t *barrier,
             std::atomic<bool> *exit_flag);

    thread_t(thread_t &&other) = default;

    static thread_t *self();

    target_id_t id() const;

    message_hub_t *hub();
    dispatcher_t *dispatcher();
    local_target_t *target();
    events_t *events();

    wait_object_t *shutdown_event();

    void exit();

    bool operator ==(const thread_t &other) const;

private:
    friend class shutdown_handler_t; // For updating the shutdown cond

    void main();

    message_hub_t m_hub;
    events_t m_events;
    dispatcher_t m_dispatch;
    local_target_t m_target;

    shutdown_t *m_shutdown;
    thread_barrier_t *m_barrier;
    std::atomic<bool> *m_exit_flag;

    event_t m_shutdown_event;

    std::thread m_thread;

    thread_local static thread_t *s_instance;
};

} // namespace indecorous

namespace std {

template <> struct hash<indecorous::thread_t> {
    size_t operator () (const indecorous::thread_t &thread) const {
        return std::hash<indecorous::target_id_t>()(thread.id());
    }
};

} // namespace std

#endif // CORO_THREAD_HPP_
