#ifndef SYNC_MULTIPLE_WAIT_HPP_
#define SYNC_MULTIPLE_WAIT_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class multiple_wait_callback_t : private wait_callback_t {
public:
    multiple_wait_callback_t(wait_object_t *obj, multiple_waiter_t *owner) :
            m_obj(obj), m_owner(owner) {
        m_obj->addWait(this);
    }

    ~multiple_wait_callback_t() {
        m_obj->removeWait(this);
    }

private:
    void wait_callback(wait_result_t result) {
        m_owner->done(result);
    }

    wait_object_t *m_obj;
    multiple_waiter_t *m_owner;
};

template <typename... Args>
void wait_any(Args &&...args) {
    coro_t *self = coro_t::self();
    auto waits[] = { wait_any_callback_t(args, self)... };
    coro_t::wait();
}

template <typename... Args>
void wait_any_interruptible(Args &&...args, wait_object_t *interruptor) {

}

template <typename... Args>
void wait_all(Args &&...args) {

}

template <typename... Args>
void wait_all_interruptible(Args &&...args, wait_object_t *interruptor) {

}

} // namespace indecorous

#endif // SYNC_MULTIPLE_WAIT_HPP_
