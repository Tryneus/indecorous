#include "common.hpp"

#include "coro/coro.hpp"

namespace indecorous {

coro_t *coro_self() {
    return coro_t::self();
}

void coro_wait() {
    coro_t::wait();
}

} // namespace indecorous
