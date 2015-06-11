#include "coro/shutdown.hpp"

#include <cassert>

#include "coro/thread.hpp"
#include "rpc/hub_data.hpp"
#include "rpc/target.hpp"

namespace indecorous {

shutdown_t::shutdown_t() :
    m_active_count(0) { }

struct init_stop_t : public handler_t<init_stop_t> {
    static void call() {
        thread_t::self()->m_stop_event.set();
    }
};
IMPL_UNIQUE_HANDLER(init_stop_t);

struct finish_stop_t : public handler_t<finish_stop_t> {
    static void call() {
        thread_t::self()->m_stop_immediately = true;
    }
};
IMPL_UNIQUE_HANDLER(finish_stop_t);

void shutdown_t::shutdown(message_hub_t *hub) {
    size_t calls = hub->broadcast_local_noreply<init_stop_t>();
    // This is called outside the context of a thread_t, so update manually
    update(calls - 1);
}

void shutdown_t::update(int64_t active_delta) {
    uint64_t res = m_active_count.fetch_add(active_delta);
    debugf("shutdown_t updated with %" PRIi64 " to %" PRIu64, active_delta, res + active_delta);

    bool done = ((res + active_delta) == 0);
    if (done && res != 0) {
        thread_t::self()->hub()->broadcast_local_noreply<finish_stop_t>();
    }
}

void shutdown_t::reset(uint64_t initial_count) {
    debugf("shutdown_t initialized with %" PRIu64, initial_count);
    assert(m_active_count.load() == 0);
    // The extra 1 is a dummy task to prevent shutdown until `shutdown` is called
    m_active_count.store(initial_count + 1);
}

} // namespace indecorous
