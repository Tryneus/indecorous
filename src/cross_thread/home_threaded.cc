#include "cross_thread/home_threaded.hpp"

#include "coro/thread.hpp"

namespace indecorous {

home_threaded_t::home_threaded_t() : m_home_thread(thread_t::self()->id()) { }
home_threaded_t::~home_threaded_t() {
}

size_t home_threaded_t::home_thread() const {
    return m_home_thread;
}

void home_threaded_t::assert_thread() const {
    assert(m_home_thread == thread_t::self()->id());
}

} // namespace indecorous
