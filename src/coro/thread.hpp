#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <atomic>
#include <thread>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "coro/coro.hpp"
#include "coro/events.hpp"
#include "rpc/hub.hpp"
#include "sync/event.hpp"

namespace indecorous {

class scheduler_t;
class shutdown_t;
class thread_barrier_t;

class thread_t {
public:
    thread_t(size_t _id,
             shutdown_t *shutdown,
             thread_barrier_t *barrier,
             std::atomic<bool> *close_flag);

    static thread_t *self();

    size_t id() const;

    message_hub_t *hub();
    dispatcher_t *dispatcher();
    events_t *events();

    waitable_t *stop_event();

    void join();

private:
    void main();

    size_t m_id;

    friend class target_t; // For immediately noting a local RPC
    shutdown_t *m_shutdown;
    thread_barrier_t *m_barrier;
    std::atomic<bool> *m_close_flag;

    std::thread m_thread;

    friend class shutdown_rpc_t; // For updating the stop event and stop flag
    event_t m_stop_event;
    bool m_stop_immediately;

    message_hub_t m_hub;
    events_t m_events;
    dispatcher_t m_dispatch;

    thread_local static thread_t *s_instance;

    DISABLE_COPYING(thread_t);
};

} // namespace indecorous

#endif // CORO_THREAD_HPP_
