#ifndef SYNC_DRAINER_HPP_
#define SYNC_DRAINER_HPP_

#include <cstddef>

#include "sync/wait_object.hpp"

namespace indecorous {

class drainer_t;

class drainer_lock_t : public wait_object_t {
public:
    drainer_lock_t(drainer_lock_t &&other);
    drainer_lock_t(const drainer_lock_t &other);
    ~drainer_lock_t();

    bool draining() const;

private:
    friend class drainer_t;
    drainer_lock_t(drainer_t *parent);

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    drainer_t *m_parent;
};

class drainer_t : public wait_object_t {
public:
    drainer_t();
    ~drainer_t();

    bool draining() const;

    drainer_lock_t lock();

private:
    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    friend class drainer_lock_t;
    size_t m_outstanding_locks;

    // List of waiters waiting for the drainer to start draining
    intrusive_list_t<wait_callback_t> m_start_drain_waiters;

    // Waiters waiting for the drainer to finish draining (in the destructor)
    wait_callback_t *m_finish_drain_waiter;
};

} // namespace indecorous

#endif // SYNC_DRAINER_HPP_
