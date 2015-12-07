#include "cross_thread/ct_mutex.hpp"

#include "coro/thread.hpp"
#include "rpc/handler.hpp"
#include "rpc/target.hpp"

namespace indecorous {

SERIALIZABLE_POINTER(cross_thread_mutex_t);
SERIALIZABLE_POINTER(cross_thread_mutex_acq_t);

class cross_thread_mutex_callback_t {
public:
    DECLARE_STATIC_RPC(notify)(cross_thread_mutex_t *, cross_thread_mutex_acq_t *) -> void;
};

cross_thread_mutex_acq_t::cross_thread_mutex_acq_t(cross_thread_mutex_t *parent) :
        m_has(false), m_parent(parent), m_waiters() {
    m_parent->add_lock(this);
}

cross_thread_mutex_acq_t::~cross_thread_mutex_acq_t() {
    if (m_parent != nullptr) {
        m_parent->release_lock(this);
        m_waiters.clear([] (auto w) { w->wait_done(wait_result_t::ObjectLost); });
    }
}

bool cross_thread_mutex_acq_t::has() const {
    return m_has;
}

void cross_thread_mutex_acq_t::ready() {
    m_has = true;
    m_waiters.clear([] (auto w) { w->wait_done(wait_result_t::Success); });
}

void cross_thread_mutex_acq_t::add_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    if (m_has) {
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void cross_thread_mutex_acq_t::remove_wait(wait_callback_t *cb) {
    assert(m_parent != nullptr);
    m_waiters.remove(cb);
}

cross_thread_mutex_t::cross_thread_mutex_t() :
    m_spinlock(),
    m_locks() { }

cross_thread_mutex_t::~cross_thread_mutex_t() {
    m_spinlock.lock();
    assert(m_locks.empty());
    m_spinlock.unlock();
}

void cross_thread_mutex_t::add_lock(cross_thread_mutex_acq_t *lock) {
    bool ready = false;

    m_spinlock.lock();
    m_locks.push_back(lock);
    ready = (m_locks.front() == lock);
    m_spinlock.unlock();

    if (ready) {
        lock->ready();
    }
}

void cross_thread_mutex_t::release_lock(cross_thread_mutex_acq_t *lock) {
    cross_thread_mutex_acq_t *next = nullptr;
    m_spinlock.lock();
    if (m_locks.front() == lock) {
        m_locks.pop_front();
        next = m_locks.front();
    } else {
        m_locks.remove(lock);
    }
    m_spinlock.unlock();

    if (next != nullptr) {
        target_t *target = thread_t::self()->hub()->target(next->home_thread());
        target->call_noreply<cross_thread_mutex_callback_t::notify>(this, std::move(next)); // TODO: should we really need this std::move here?
    }
}

void cross_thread_mutex_t::cross_thread_notification(cross_thread_mutex_acq_t *lock) {
    // We need to check for the case that the acq_t was destroyed after sending
    // the notify RPC, but before the RPC was run.  If the acq_t is still at the
    // front of the queue, we do the unlock.
    m_spinlock.lock();
    bool do_unlock = (m_locks.front() == lock);
    m_spinlock.unlock();
    if (do_unlock) {
        lock->ready();
    }
}

IMPL_STATIC_RPC(cross_thread_mutex_callback_t::notify)(cross_thread_mutex_t *m,
                                                       cross_thread_mutex_acq_t *acq) -> void {
    m->cross_thread_notification(acq);
}

} // namespace indecorous
