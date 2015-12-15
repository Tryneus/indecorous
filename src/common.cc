#include "common.hpp"

#include "coro/thread.hpp"

namespace indecorous {

int64_t thread_self_id() {
    thread_t *t = thread_t::self();
    return t == nullptr ? -1 : t->target()->id().value();
}

} // namespace indecorous
