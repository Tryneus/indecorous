#include "coro/shutdown.hpp"

#include <sys/eventfd.h>

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
        t.hub()->self_target()->call_noreply<shutdown_handler_t>();
    }
    update(threads->size()); // Count the rpcs as active coroutines
}

bool shutdown_t::update(int64_t active_delta) {
    uint64_t res = m_active_count.fetch_add(active_delta);
    debugf("shutdown_t updated with %" PRIi64 " to %" PRIu64, active_delta, res + active_delta);

    bool done = ((res + active_delta) == 0);
    if (done && active_delta != 0) {
        debugf("setting shutdown file event");
        m_file_event.set();
    }
    return done;
}

void shutdown_t::reset(uint64_t initial_count) {
    debugf("shutdown_t initialized with %" PRIu64, initial_count);
    assert(m_active_count.load() == 0);
    m_file_event.reset();
    m_active_count.store(initial_count);
}

file_callback_t *shutdown_t::get_file_event() {
    return &m_file_event;
}

shutdown_file_event_t::shutdown_file_event_t() :
        file_callback_t(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
        m_scoped_fd(fd()) {
    assert(m_scoped_fd.is_valid());
}

void shutdown_file_event_t::set() {
    uint64_t value = 1;
    int res = ::write(fd.get(), &value, sizeof(value));
    assert(res == sizeof(value));
}

void shutdown_file_event_t::reset() {
    uint64_t value;
    int res = ::read(fd.get(), &value, sizeof(value));
    assert(res == sizeof(value));
    assert(res == 1);
}

void shutdown_file_event_t::file_callback(wait_result_t result) {
    assert(result == wait_result_t::Success);
}

} // namespace indecorous
