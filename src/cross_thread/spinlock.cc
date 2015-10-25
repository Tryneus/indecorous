#include "cross_thread/spinlock.hpp"

namespace indecorous {

spinlock_t::spinlock_t() {
    m_locked = ATOMIC_FLAG_INIT;
}
spinlock_t::~spinlock_t() {
    assert(!m_locked.test_and_set(std::memory_order_acquire));
}

void spinlock_t::lock() {
    while (m_locked.test_and_set(std::memory_order_acquire)) { }
}

void spinlock_t::unlock() {
    m_locked.clear(std::memory_order_release);
}

spinlock_acq_t::spinlock_acq_t(spinlock_t *lock) : m_lock(lock) {
    m_lock->lock();
}

spinlock_acq_t::~spinlock_acq_t() {
    m_lock->unlock();
}

} // namespace indecorous
