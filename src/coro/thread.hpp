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
class shared_registry_t;
class shutdown_t;

class thread_t {
public:
    thread_t(size_t _id, scheduler_t *parent);

    static thread_t *self();

    size_t id() const;

    message_hub_t *hub();
    dispatcher_t *dispatcher();
    events_t *events();

    waitable_t *stop_event();

    void join();

private:
    void main();

    // For deserializing shared_var_t references to the shared_registry_t
    template <typename T> friend struct serializer_t;
    shared_registry_t *get_shared_registry();

    // For immediately noting a local RPC
    friend class target_t;
    shutdown_t *get_shutdown();

    size_t m_id;
    scheduler_t *m_parent;
    std::thread m_thread;

    // For updating the stop event and stop flag
    friend class shutdown_rpc_t;
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
