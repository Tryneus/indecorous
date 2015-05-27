#include "sync/wait_object.hpp"

#include "coro/coro.hpp"

namespace indecorous {

// If a wait failed, throw an appropriate exception so the user can handle it
void check_wait_result(wait_result_t result) {
    switch (result) {
    case wait_result_t::Success:
        break;
    case wait_result_t::Interrupted:
        throw wait_interrupted_exc_t();
    case wait_result_t::ObjectLost:
        throw wait_object_lost_exc_t();
    default:
        throw wait_error_exc_t("unrecognized error while waiting");
    }
}

void coro_wait(intrusive_list_t<wait_callback_t> *list) {
    coro_t *self = coro_t::self();
    list->push_back(self->wait_callback());
    self->wait();
}

} // namespace indecorous

