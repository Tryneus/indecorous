#ifndef SYNC_DRAINER_HPP_
#define SYNC_DRAINER_HPP_

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class drainer_t;

class drainer_lock_t final : public waitable_t,
                             public wait_callback_t,
                             public intrusive_node_t<drainer_lock_t> {
public:
    drainer_lock_t(drainer_lock_t &&other);
    drainer_lock_t(const drainer_lock_t &other);
    drainer_lock_t &operator = (const drainer_lock_t &other);
    ~drainer_lock_t();

    bool draining() const;

    // Do not use this, just do your standard RAII stuff
    void release();

private:
    friend class drainer_t;
    explicit drainer_lock_t(drainer_t *parent);

    // wait_callback_t implementation - called when m_waitable has completed
    void wait_done(wait_result_t result) override final;
    void object_moved(waitable_t *new_ptr) override final;

    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    intrusive_list_t<wait_callback_t> m_waiters;
    drainer_t *m_parent;
};

class drainer_t final : public waitable_t {
public:
    drainer_t(drainer_t &&other);
    drainer_t();
    ~drainer_t();

    bool draining() const;
    void drain();

    drainer_lock_t lock();

private:
    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    friend class drainer_lock_t;

    void add_lock(drainer_lock_t *l);
    void remove_lock(drainer_lock_t *l);

    // List of extant locks on the drainer, which will block draining
    intrusive_list_t<drainer_lock_t> m_locks;

    // List of waiters waiting for the drainer to start draining
    intrusive_list_t<wait_callback_t> m_start_drain_waiters;

    // Waiters waiting for the drainer to finish draining
    intrusive_list_t<wait_callback_t> m_finish_drain_waiters;

    bool m_draining;

    DISABLE_COPYING(drainer_t);
};

} // namespace indecorous

#endif // SYNC_DRAINER_HPP_
