#ifndef CROSS_THREAD_CT_MUTEX_HPP_
#define CROSS_THREAD_CT_MUTEX_HPP_

#include "cross_thread/spinlock.hpp"

namespace indecorous {

class cross_thread_mutex_acq_t : public home_threaded_t, public waitable_t {
public:
    ~cross_thread_mutex_acq_t();
    bool has() const;

private:
    friend class cross_thread_mutex_t;
    cross_thread_mutex_acq_t(cross_thread_mutex_t *parent);
    void ready();

    void add_wait(wait_callback_t *cb) override final;
    void remove_wait(wait_callback_t *cb) override final;

    bool m_has;
    cross_thread_mutex_t *m_parent;
    intrusive_list_t<wait_callback_t> m_waiters;

    DISABLE_COPYING(cross_thread_mutex_acq_t);
};

class cross_thread_mutex_t {
public:
    cross_thread_mutex_t();
    ~cross_thread_mutex_t();

    cross_thread_mutex_acq_t start_acq();

private:
    friend class cross_thread_mutex_acq_t;
    void add_lock(cross_thread_mutex_acq_t *lock);
    void release_lock(cross_thread_mutex_acq_t *lock);

    friend class cross_thread_mutex_callback_t;
    void cross_thread_notification(cross_thread_mutex_acq_t *lock);

    spinlock_t m_spinlock;
    intrusive_list_t<cross_thread_mutex_acq_t> m_locks;
};

#endif // CROSS_THREAD_CT_MUTEX_HPP_
