#include "sync/wait_object.hpp"

#include "coro/coro.hpp"

namespace indecorous {

void coro_wait(intrusive_list_t<wait_callback_t> *list) {
    list->push_back(coro_t::self()->wait_callback());
    coro_t::wait();
}

} // namespace indecorous

