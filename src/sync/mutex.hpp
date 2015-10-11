#ifndef SYNC_MUTEX_HPP_
#define SYNC_MUTEX_HPP_

#include "common.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class mutex_t;
class wait_callback_t;

class mutex_acq_t final : public waitable_t, public intrusive_node_t<mutex_acq_t> {
public:
    mutex_acq_t(mutex_acq_t &&other);
    ~mutex_acq_t();

    bool has() const;
private:
    friend class mutex_t;
    explicit mutex_acq_t(mutex_t *parent);
    void ready();

    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    mutex_t *m_parent;
    intrusive_list_t<wait_callback_t> m_waiters;

    DISABLE_COPYING(mutex_acq_t);
};

class mutex_t {
public:
    mutex_t();
    mutex_t(mutex_t &&other);
    ~mutex_t();

    mutex_acq_t start_acq();

private:
    friend class mutex_acq_t;
    void add_lock(mutex_acq_t *lock);
    void release_lock(mutex_acq_t *lock);

    intrusive_list_t<mutex_acq_t> m_pending_locks;

    DISABLE_COPYING(mutex_t);
};

} // namespace indecorous

#endif // SYNC_MUTEX_HPP_
