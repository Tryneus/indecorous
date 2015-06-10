#include "common.hpp"

#include "coro/thread.hpp"

namespace indecorous {

int32_t thread_self_id() {
    thread_t *t = thread_t::self();
    return t == nullptr ? -1 : t->id();
}

} // namespace indecorous
