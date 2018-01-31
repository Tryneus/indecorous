#include "coro/shutdown.hpp"

#include "coro/thread.hpp"
#include "rpc/handler.hpp"

namespace indecorous {

class shutdown_rpc_t {
public:
    DECLARE_STATIC_RPC(begin_shutdown)() -> void;
    DECLARE_STATIC_RPC(finish_shutdown)() -> void;
};

IMPL_STATIC_RPC(shutdown_rpc_t::begin_shutdown)() -> void {
    logDebug("Beginning shutdown");
    thread_t::self()->begin_shutdown();
}

IMPL_STATIC_RPC(shutdown_rpc_t::finish_shutdown)() -> void {
    logDebug("Finishing shutdown");
    thread_t::self()->finish_shutdown();
}

shutdown_t::shutdown_t(std::vector<target_t *> targets) :
   m_targets(std::move(targets)),
   m_finish_sent(true),
   m_active_count(0) { }

void shutdown_t::reset(size_t initial_count) {
   // The extra 1 is a dummy task to prevent shutdown until `begin_shutdown` is called
    m_active_count = initial_count + 1;
    m_finish_sent = false;
}

void shutdown_t::begin_shutdown() {
    // This is called outside the context of a thread_t, so update manually
    update(m_targets.size() - 1);
    for (auto &&t : m_targets) {
        t->call_noreply<shutdown_rpc_t::begin_shutdown>();
    }
}

void shutdown_t::update(int64_t active_delta) {
    uint64_t res = m_active_count.fetch_add(active_delta);

    bool done = ((res + active_delta) == 0);
    if (done && res != 0 && !m_finish_sent.load()) {
        logInfo("All tasks finished, completing shutdown");
        m_active_count.fetch_add(m_targets.size());
        m_finish_sent.store(true);
        for (auto &&t : m_targets) {
            t->call_noreply<shutdown_rpc_t::finish_shutdown>();
        }
    }
}

} // namespace indecorous
