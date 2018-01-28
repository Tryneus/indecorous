#ifndef SYNC_INTERRUPTOR_HPP_
#define SYNC_INTERRUPTOR_HPP_

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class waitable_t;

class interruptor_t final : public waitable_t,
                            public wait_callback_t,
                            public intrusive_node_t<interruptor_t> {
public:
    explicit interruptor_t(waitable_t *waitable);
    explicit interruptor_t(interruptor_t *parent_interruptor);
    interruptor_t(interruptor_t &&other);
    ~interruptor_t();

    bool triggered() const;

private:
    // waitable_t implementation - called by interruptors down the chain or by a coroutine wait
    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    // wait_callback_t implementation - called when m_waitable has completed
    void wait_done(wait_result_t result) override final;
    void object_moved(waitable_t *new_ptr) override final;

    void handle_triggered();

    // Use this rather than inherit from it directly to avoid multiple inheritance
    // of wait_callback_t.
    class prev_waiter_t final : public wait_callback_t {
    public:
        prev_waiter_t(interruptor_t *parent, waitable_t *prev_interruptor);
        prev_waiter_t(prev_waiter_t &&other);
        ~prev_waiter_t();
    private:
        // wait_callback_t implementation - called when the previous interruptor has triggered
        void wait_done(wait_result_t result) override final;
        void object_moved(waitable_t *new_ptr) override final;

        interruptor_t *m_parent;
        waitable_t *m_prev_interruptor;

        DISABLE_COPYING(prev_waiter_t);
    };

    bool m_triggered;
    waitable_t *m_waitable;
    intrusive_list_t<wait_callback_t> m_waiters;
    prev_waiter_t m_prev_waiter;

    DISABLE_COPYING(interruptor_t);
};

// Instantiating an interruptor_clear_t will disable all current interruptors on the
// running coroutine until the interruptor_clear_t is destroyed.  New interruptors can
// still be registered and will work normally.  Child coroutines will still inherit the
// interruptor chain and update accordingly when the interruptor_clear_t is destroyed.
class interruptor_clear_t {
public:
    interruptor_clear_t();
    ~interruptor_clear_t();

private:
    intrusive_list_t<interruptor_t> m_old_interruptors;

    DISABLE_COPYING(interruptor_clear_t);
};

} // namespace indecorous

#endif // SYNC_INTERRUPTOR_HPP_
