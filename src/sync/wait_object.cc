#include "sync/wait_object.hpp"

#include "coro/coro.hpp"
#include "coro/thread.hpp"

namespace indecorous {

const char *wait_result_str(wait_result_t res) {
    switch (res) {
    case wait_result_t::Success:
        return "Success";
    case wait_result_t::Interrupted:
        return "Interrupted";
    case wait_result_t::ObjectLost:
        return "ObjectLost";
    default: UNREACHABLE();
    }
}

// If a wait failed, throw an appropriate exception so the user can handle it
void check_wait_result(wait_result_t result) {
    switch (result) {
    case wait_result_t::Success:
        break;
    case wait_result_t::Interrupted:
        throw wait_interrupted_exc_t();
    case wait_result_t::ObjectLost:
        throw wait_object_lost_exc_t();
    default: UNREACHABLE();
    }
}

void waitable_t::wait() {
    // This will mix in the interruptor (if set for this coroutine)
    multiple_waiter_t multiple_wait(multiple_waiter_t::wait_type_t::ANY, 1);
    multiple_wait_callback_t self_wait(this, &multiple_wait);
    multiple_wait.wait();
}

} // namespace indecorous

