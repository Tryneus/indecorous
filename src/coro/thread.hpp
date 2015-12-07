#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <atomic>
#include <memory>
#include <thread>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "coro/events.hpp"
#include "rpc/hub.hpp"
#include "sync/event.hpp"

namespace indecorous {

class scheduler_t;
class shared_registry_t;
class shutdown_t;

class thread_t {
public:
    thread_t(scheduler_t *parent,
             target_t *io_target,
             std::function<void()> inner_main,
             std::function<void()> coro_pull);

    static thread_t *self() { return s_instance; }

    void join();

    target_t *target() { return &m_direct_target; }
    message_hub_t *hub() { return &m_hub; }
    events_t *events() { return &m_events; }
    dispatcher_t *dispatcher() { return m_dispatcher.get(); }
    event_t *shutdown_event() { return &m_shutdown_event; }

    size_t queue_length() const { return m_direct_stream.size(); }

protected:
    void main();

    // For initiating and completing stop of a thread
    friend class shutdown_rpc_t;
    void begin_shutdown();
    void finish_shutdown();

    // For immediately noting a local RPC
    friend class target_t;
    void note_local_rpc();

    // For deserializing shared_var_t references to the shared_registry_t
    template <typename T> friend struct serializer_t;
    shared_registry_t *get_shared_registry();

    scheduler_t * const m_parent;

    local_stream_t m_direct_stream;
    local_target_t<local_stream_t> m_direct_target;

    message_hub_t m_hub;
    events_t m_events;
    std::unique_ptr<dispatcher_t> m_dispatcher;

    event_t m_shutdown_event;
    bool m_stop_immediately;
    std::function<void()> m_inner_main;
    std::function<void()> m_coro_pull;
    std::thread m_thread;

    thread_local static thread_t *s_instance;

    DISABLE_COPYING(thread_t);
};

class coro_thread_t {
public:
    coro_thread_t(scheduler_t *parent,
                  target_t *io_target);

    thread_t *thread() { return &m_thread; }

private:
    void inner_main();

    thread_t m_thread;

    DISABLE_COPYING(coro_thread_t);
};

class io_thread_t {
public:
    io_thread_t(scheduler_t *parent,
                target_t *io_target,
                io_stream_t *io_stream);

    thread_t *thread() { return &m_thread; }

private:
    void inner_main();
    [[noreturn]] void coro_pull();

    io_stream_t * const m_io_stream;
    event_t m_ready_for_next;
    thread_t m_thread;

    DISABLE_COPYING(io_thread_t);
};

} // namespace indecorous

#endif // CORO_THREAD_HPP_
