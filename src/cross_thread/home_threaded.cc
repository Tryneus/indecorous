#include "cross_thread/home_threaded.hpp"

#include "coro/thread.hpp"
#include "rpc/target.hpp"

namespace indecorous {

home_threaded_t::home_threaded_t() : m_home_thread(thread_t::self()->target()->id()) { }
home_threaded_t::~home_threaded_t() {
}

target_id_t home_threaded_t::home_thread() const {
    return m_home_thread;
}

void home_threaded_t::assert_thread() const {
    assert(m_home_thread == thread_t::self()->target()->id());
}

} // namespace indecorous
