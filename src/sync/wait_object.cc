#include "sync/wait_object.hpp"

#include "coro/coro.hpp"
#include "coro/thread.hpp"

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

void wait_object_t::wait() {
    dispatcher_t *dispatch = thread_t::self()->dispatcher();
    coro_t *self = dispatch->m_running;
    assert(!self->in_a_list());
    add_wait(self->wait_callback());
    if (self->in_a_list()) {
        // The wait object was immediately ready, just unenqueue ourselves and continue
        dispatch->m_run_queue.remove(self);
        wait_result_t result = self->m_wait_result;
        self->m_wait_result = wait_result_t::Success;
        check_wait_result(result);
    } else {
        // We have to wait until the object is ready, after which we'll be reenqueued
        self->wait();
    }
}

} // namespace indecorous

