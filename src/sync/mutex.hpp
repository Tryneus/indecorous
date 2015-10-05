#ifndef SYNC_MUTEX_HPP_
#define SYNC_MUTEX_HPP_

#include "common.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class mutex_t;
class wait_callback_t;

class mutex_lock_t : public intrusive_node_t<mutex_lock_t> {
public:
    mutex_lock_t(mutex_lock_t &&other);
    ~mutex_lock_t();
private:
    friend class mutex_t;
    explicit mutex_lock_t(mutex_t *parent);

    mutex_t *m_parent;
    wait_callback_t *m_coro_cb;

    DISABLE_COPYING(mutex_lock_t);
};

// The mutex is not publicly implemented as a waitable_t so you cannot
// wait_all on them and unintentionally deadlock.
// TODO: double check this to see if anything can be done
class mutex_t {
public:
    mutex_t(mutex_t &&other);
    mutex_t();
    ~mutex_t();

    mutex_lock_t lock();

private:
    friend class mutex_lock_t;
    mutex_lock_t *m_lock;
    intrusive_list_t<mutex_lock_t> m_pending_locks;

    DISABLE_COPYING(mutex_t);
};

} // namespace indecorous

#endif // SYNC_MUTEX_HPP_
