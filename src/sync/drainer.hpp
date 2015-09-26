#ifndef SYNC_DRAINER_HPP_
#define SYNC_DRAINER_HPP_

#include <cstddef>

#include "sync/wait_object.hpp"

namespace indecorous {

class drainer_t;

class drainer_lock_t final : public waitable_t, public intrusive_node_t<drainer_lock_t> {
public:
    drainer_lock_t(drainer_lock_t &&other);
    drainer_lock_t(const drainer_lock_t &other);
    drainer_lock_t &operator = (const drainer_lock_t &other);
    ~drainer_lock_t();

    bool draining() const;

private:
    friend class drainer_t;
    explicit drainer_lock_t(drainer_t *parent);

    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    drainer_t *m_parent;
};

class drainer_t final : public waitable_t {
public:
    drainer_t(drainer_t &&other);
    drainer_t();
    ~drainer_t();

    bool draining() const;

    drainer_lock_t lock();

private:
    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    friend class drainer_lock_t;
    // List of extant locks on the drainer, which will block draining
    intrusive_list_t<drainer_lock_t> m_locks;

    // List of waiters waiting for the drainer to start draining
    intrusive_list_t<wait_callback_t> m_start_drain_waiters;

    // Waiters waiting for the drainer to finish draining (in the destructor)
    wait_callback_t *m_finish_drain_waiter;

    DISABLE_COPYING(drainer_t);
};

} // namespace indecorous

#endif // SYNC_DRAINER_HPP_
