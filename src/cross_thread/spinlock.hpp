#ifndef CROSS_THREAD_SPINLOCK_HPP_
#define CROSS_THREAD_SPINLOCK_HPP_

#include <atomic>

namespace indecorous {

class spinlock_t {
public:
    spinlock_t();
    ~spinlock_t();

    void lock();
    void unlock();

private:
    std::atomic_flag m_locked;
};

class spinlock_acq_t {
public:
    spinlock_acq_t(spinlock_t *lock);
    ~spinlock_acq_t();
private:
    spinlock_t *m_lock;
};

} // namespace indecorous

#endif // CROSS_THREAD_SPINLOCK_HPP_
