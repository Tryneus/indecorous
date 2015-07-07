#ifndef SYNC_MUTEX_HPP_
#define SYNC_MUTEX_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class mutex_t;

class mutex_lock_t : public intrusive_node_t<mutex_lock_t> {
public:
    mutex_lock_t(mutex_lock_t &&other);
    ~mutex_lock_t();
private:
    friend class mutex_t;
    mutex_lock_t(mutex_t *parent, waitable_t *interruptor);

    mutex_t *m_parent;
    wait_callback_t *m_coro_cb;
};

// The mutex is not publicly implemented as a waitable_t so you cannot
// wait_all on them and unintentionally deadlock.
class mutex_t {
public:
    mutex_t(mutex_t &&other);
    mutex_t();
    ~mutex_t();

    mutex_lock_t lock();
    mutex_lock_t lock(waitable_t *interruptor);

private:
    friend class mutex_lock_t;
    mutex_lock_t *m_lock;
    intrusive_list_t<mutex_lock_t> m_pending_locks;
};

} // namespace indecorous

#endif // SYNC_MUTEX_HPP_
