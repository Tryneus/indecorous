#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <atomic>
#include <thread>

#include "containers/intrusive.hpp"
#include "coro/coro.hpp"
#include "coro/events.hpp"
#include "coro/shutdown.hpp"
#include "rpc/id.hpp"
#include "rpc/hub.hpp"
#include "sync/event.hpp"

namespace indecorous {

class message_hub_t;
class scheduler_t;
class thread_barrier_t;

class thread_t {
public:
    thread_t(size_t _id,
             shutdown_t *shutdown,
             thread_barrier_t *barrier,
             std::atomic<bool> *exit_flag);

    static thread_t *self();

    size_t id() const;

    message_hub_t *hub();
    dispatcher_t *dispatcher();
    events_t *events();

    wait_object_t *shutdown_event();

    void exit();

private:
    friend struct shutdown_handler_t; // For updating the shutdown event

    void main();

    size_t m_id;

    shutdown_t *m_shutdown;
    thread_barrier_t *m_barrier;
    std::atomic<bool> *m_exit_flag;

    std::thread m_thread;

    event_t m_shutdown_event;

    message_hub_t m_hub;
    events_t m_events;
    dispatcher_t m_dispatch;

    thread_local static thread_t *s_instance;
};

} // namespace indecorous

#endif // CORO_THREAD_HPP_
