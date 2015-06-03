#include "coro/shutdown.hpp"

#include <cassert>
#include <unordered_set>

#include "coro/thread.hpp"
#include "rpc/hub_data.hpp"
#include "rpc/target.hpp"

namespace indecorous {

shutdown_t::shutdown_t() :
        m_active_count(0) { }

struct shutdown_handler_t : public handler_t<shutdown_handler_t> {
    static void call() {
        thread_t::self()->m_shutdown_event.set();
    }
};
IMPL_UNIQUE_HANDLER(shutdown_handler_t);

void shutdown_t::shutdown(std::list<thread_t> *threads) {
    // Iterate over threads, send RPC to trigger the shutdown cond
    for (auto &&t: *threads) {
        t.hub()->self_target()->noreply_call<shutdown_handler_t>();
    }
}

bool shutdown_t::update(int64_t active_delta) {
    uint64_t res = m_active_count.fetch_add(active_delta);
    return (res + active_delta) == 0;
}

void shutdown_t::reset() {
    assert(m_active_count.load() == 0);
}

} // namespace indecorous
