#include "common.hpp"

#include "coro/thread.hpp"

namespace indecorous {

size_t thread_self_id() {
    thread_t *t = thread_t::self();
    assert(t != nullptr);
    return t->id();
}

} // namespace indecorous
