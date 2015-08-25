#include "coro/shutdown.hpp"

#include <cassert>

#include "coro/thread.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"

namespace indecorous {

shutdown_t::shutdown_t(size_t thread_count) :
    m_finish_sent(true), m_active_count(0), m_thread_count(thread_count) { }

void shutdown_t::shutdown(message_hub_t *hub) {
    update(m_thread_count - 1, hub); // This is called outside the context of a dispatch_t, so update manually
    GUARANTEE(hub->broadcast_local_noreply<shutdown_t::init_stop>() == m_thread_count);
}

void shutdown_t::update(int64_t active_delta, message_hub_t *hub) {
    uint64_t res = m_active_count.fetch_add(active_delta);

    bool done = ((res + active_delta) == 0);
    if (done && res != 0) {
        if (!m_finish_sent.load()) {
            m_active_count.fetch_add(m_thread_count);
            m_finish_sent.store(true);
            hub->broadcast_local_noreply<shutdown_t::finish_stop>();
        }
    }
}

void shutdown_t::reset(uint64_t initial_count) {
    // The extra 1 is a dummy task to prevent shutdown until `shutdown` is called
    m_finish_sent.store(false);
    m_active_count.store(initial_count + 1);
}

IMPL_STATIC_RPC(shutdown_t::init_stop) -> void {
    thread_t::self()->m_stop_event.set();
}

IMPL_STATIC_RPC(shutdown_t::finish_stop) -> void {
    thread_t::self()->m_stop_immediately = true;
}

} // namespace indecorous
