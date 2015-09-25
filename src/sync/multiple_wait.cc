#include "sync/multiple_wait.hpp"

#include "coro/coro.hpp"

namespace indecorous {

multiple_waiter_t::multiple_waiter_t(multiple_waiter_t::wait_type_t type,
                                     size_t total,
                                     waitable_t *interruptor) :
        m_owner_coro(coro_t::self()),
        m_interruptor(interruptor),
        m_ready(false),
        m_waiting(false),
        m_needed(type == wait_type_t::ALL ? total : 1),
        m_error_result(wait_result_t::Success) {
    if (m_interruptor != nullptr) {
        m_interruptor->add_wait(this);
    }
}

multiple_waiter_t::~multiple_waiter_t() {
    if (in_a_list()) {
        assert(m_interruptor != nullptr);
        m_interruptor->remove_wait(this);
    }
}

void multiple_waiter_t::wait() {
    // Handle the case of completion before the wait
    if (m_error_result != wait_result_t::Success) {
        check_wait_result(m_error_result);
        assert(false);
    } else if (!m_ready) {
        m_waiting = true;
        m_owner_coro->wait();
    }
}

void multiple_waiter_t::item_finished(wait_result_t result) {
    if (!m_ready) {
        if (result != wait_result_t::Success) {
            ready(result);
        } else if (--m_needed == 0) {
            ready(wait_result_t::Success);
        }
    } else if (result != wait_result_t::Success) {
        m_error_result = result;
    }
}

void multiple_waiter_t::ready(wait_result_t result) {
    m_ready = true;
    m_error_result = result;
    if (m_waiting) {
        m_owner_coro->wait_callback()->wait_done(result);
        m_waiting = false;
    }
}

// Called by the interruptor - could be before waiting
void multiple_waiter_t::wait_done(wait_result_t result) {
    if (!m_ready) {
        if (result == wait_result_t::Success) {
            ready(wait_result_t::Interrupted);
        } else {
            ready(result);
        }
    }
}

void multiple_waiter_t::object_moved(waitable_t *new_ptr) {
    m_interruptor = new_ptr;
}

multiple_wait_callback_t::multiple_wait_callback_t(waitable_t *obj,
                                                   multiple_waiter_t *waiter) :
        m_obj(obj), m_waiter(waiter) {
    m_obj->add_wait(this);
}

multiple_wait_callback_t::multiple_wait_callback_t(waitable_t &obj,
                                                   multiple_waiter_t *waiter) :
        m_obj(&obj), m_waiter(waiter) {
    m_obj->add_wait(this);
}

multiple_wait_callback_t::~multiple_wait_callback_t() {
    if (in_a_list()) {
        m_obj->remove_wait(this);
    }
}

void multiple_wait_callback_t::wait_done(wait_result_t result) {
    m_waiter->item_finished(result);
}

void multiple_wait_callback_t::object_moved(waitable_t *new_ptr) {
    m_obj = new_ptr;
}

} // namespace indecorous
