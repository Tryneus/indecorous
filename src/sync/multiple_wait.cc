#include "sync/multiple_wait.hpp"

#include "coro/coro.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

multiple_waiter_t::multiple_waiter_t(multiple_waiter_t::wait_type_t type,
                                     size_t total) :
        m_owner_coro(coro_t::self()),
        m_interruptor(m_owner_coro->get_interruptor()),
        m_items(),
        m_ready(false),
        m_waiting(false),
        m_needed(type == wait_type_t::ALL ? total : 1),
        m_result(wait_result_t::Success) { }

multiple_waiter_t::~multiple_waiter_t() {
    // This should have been marked ready, either by completion or interruption
    assert(m_ready);
    if (in_a_list()) {
        assert(m_interruptor != nullptr);
        m_interruptor->remove_wait(this);
    }
}

void multiple_waiter_t::wait() {
    m_items.each([&] (auto cb) { cb->begin_wait(); });

    // We may be immediately ready
    if (!m_ready) {
        if (m_interruptor != nullptr) {
            m_interruptor->add_wait(this);
        }

        m_waiting = true;
        m_owner_coro->wait();
    } else {
        assert(m_ready);
        m_items.clear([&] (auto cb) { cb->cancel_wait(); });
        check_wait_result(m_result);
    }
}

void multiple_waiter_t::add_item(multiple_wait_callback_t *cb) {
    m_items.push_back(cb);
}

void multiple_waiter_t::item_finished(wait_result_t result) {
    assert(m_needed > 0);
    if (!m_ready) {
        if (result != wait_result_t::Success) {
            ready(result);
        } else if (--m_needed == 0) {
            ready(wait_result_t::Success);
        }
    } else if (result != wait_result_t::Success) {
        m_result = result;
    }
}

void multiple_waiter_t::ready(wait_result_t result) {
    assert(!m_ready);
    m_ready = true;
    m_result = result;
    if (m_waiting) {
        m_items.clear([&] (auto cb) { cb->cancel_wait(); });
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
    m_waiter->add_item(this);
}

multiple_wait_callback_t::multiple_wait_callback_t(waitable_t &obj,
                                                   multiple_waiter_t *waiter) :
        m_obj(&obj), m_waiter(waiter) {
    m_waiter->add_item(this);
}

multiple_wait_callback_t::~multiple_wait_callback_t() { }

void multiple_wait_callback_t::begin_wait() {
    m_obj->add_wait(this);
}

void multiple_wait_callback_t::cancel_wait() {
    // This could be called after `wait_done`, where m_obj would have removed us already
    if (intrusive_node_t<wait_callback_t>::in_a_list()) {
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
