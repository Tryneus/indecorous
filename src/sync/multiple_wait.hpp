#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_


#include "coro/coro.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class multiple_waiter_t : private wait_callback_t {
public:
    enum class wait_type_t { ANY, ALL };

    void wait() {
        // Handle the case of completion before the wait
        if (m_error_result != wait_result_t::Success) {
            check_wait_result(m_error_result);
            assert(false);
        } else if (!m_ready) {
            m_waiting = true;
            m_owner_coro->wait();
        }
    }

    void item_finished(wait_result_t result) {
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

private:
    multiple_waiter_t(wait_type_t type, size_t total, wait_object_t *interruptor) :
            m_owner_coro(coro_t::self()),
            m_interruptor(interruptor),
            m_ready(false),
            m_waiting(false),
            m_needed(type == wait_type_t::ALL ? total : 1),
            m_error_result(wait_result_t::Success) {
        if (m_interruptor != nullptr) {
            m_interruptor->addWait(this);
        }
    }

    void ready(wait_result_t result) {
        m_ready = true;
        m_error_result = result;
        if (m_waiting) {
            m_owner_coro->wait_callback()->wait_done(result);
            m_waiting = false;
        }
    }

    // Called by the interruptor - could be before waiting
    void wait_done(wait_result_t result) {
        if (!m_ready) {
            if (result == wait_result_t::Success) {
                ready(wait_result_t::Interrupted);
            } else {
                ready(result);
            }
        }
    }

    ~multiple_waiter_t() {
        if (in_a_list()) {
            assert(m_interruptor != nullptr);
            m_interruptor->removeWait(this);
        }
    }

    coro_t *m_owner_coro;
    wait_object_t *m_interruptor;

    bool m_ready;
    bool m_waiting;
    size_t m_needed;
    wait_result_t m_error_result;
};

class multiple_wait_callback_t : private wait_callback_t {
public:
    multiple_wait_callback_t(wait_object_t *obj,
                             multiple_waiter_t *waiter) :
            m_obj(obj), m_waiter(waiter) {
        m_obj->addWait(this);
    }

    ~multiple_wait_callback_t() {
        if (in_a_list()) {
            m_obj->removeWait(this);
        }
    }

private:
    void wait_done(wait_result_t result) {
        m_waiter->item_finished(result);
    }

    wait_object_t *m_obj;
    multiple_waiter_t *m_waiter;
};

template <typename... Args>
void wait_any(Args &&...args) {
    multiple_waiter_t waiter(multiple_waiter_t::wait_type_t::ANY, sizeof...(Args), nullptr);
    multiple_wait_callback_t waits[] = { multiple_wait_callback_t(args, &waiter)... };
    waiter.wait();
}

template <typename... Args>
void wait_any_interruptible(Args &&...args, wait_object_t *interruptor) {
    multiple_waiter_t waiter(multiple_waiter_t::wait_type_t::ANY, sizeof...(Args), interruptor);
    multiple_wait_callback_t waits[] = { multiple_wait_callback_t(args, &waiter)... };
    waiter.wait();
}

template <typename... Args>
void wait_all(Args &&...args) {
    multiple_waiter_t waiter(multiple_waiter_t::wait_type_t::ALL, sizeof...(Args), nullptr);
    multiple_wait_callback_t waits[] = { multiple_wait_callback_t(args, &waiter)... };
    waiter.wait();
}

template <typename... Args>
void wait_all_interruptible(Args &&...args, wait_object_t *interruptor) {
    multiple_waiter_t waiter(multiple_waiter_t::wait_type_t::ALL, sizeof...(Args), interruptor);
    multiple_wait_callback_t waits[] = { multiple_wait_callback_t(args, &waiter)... };
    waiter.wait();
}

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
