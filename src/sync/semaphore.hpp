#ifndef SYNC_SEMAPHORE_HPP_
#define SYNC_SEMAPHORE_HPP_

#include <cstddef>

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class semaphore_t;

// The semaphore_t provides an RAII interface as well as safe move semantics for both semaphore_t and
// for semaphore_acq_t.  The semaphore_t's capacity may be changed at runtime.  It can be increased
// trivially by calling `extend`, or it can be decreased by first acquiring the count of resources to
// be removed from the pool.
// Waiting on a semaphore is FIFO to avoid starvation.
// This does not prevent users from doing something stupid.  The things to watch out for are:
//  - Using a semaphore_acq_t after invalidation by move semantics
//    (i.e. semaphore_t::remove or semaphore_acq_t::combine)
//  - Deadlock by obtaining locks to two semaphores in alternating order in concurrent coroutines.

// Usage:
// semaphore_t sem(10);
// sem.capacity(); // == 10
//
// // semaphore_acq_t is RAII - letting one go out of scope will release its resources back to the pool
// semaphore_acq_t acq = sem.start_acq(6); // owned = 0 (or 6 if it was immediately available)
// acq.wait();                             // owned = 6
// acq.release(3);                         // owned = 3
// acq.combine(sem.start_acq(5));          // owned = 3 (or 8 if it was immediately available)
// acq.wait();                             // owned = 8
//
// sem.remove(acq); // permanently remove the owned resources from the semaphore and invalidate the acq
// sem.capacity(); // == 2

class semaphore_acq_t : public waitable_t, public intrusive_node_t<semaphore_acq_t> {
public:
    semaphore_acq_t(semaphore_acq_t &&other);
    ~semaphore_acq_t();

    size_t owned() const;
    size_t pending() const;

    void combine(semaphore_acq_t &&other);
    void release(size_t count);

private:
    friend class semaphore_t;
    semaphore_acq_t(size_t count, semaphore_t *parent);
    semaphore_acq_t(const semaphore_acq_t &) = delete;
    semaphore_acq_t &operator = (const semaphore_acq_t &) = delete;

    // Callback from the parent semaphore_t when it can be fulfilled
    void ready();
    // Callback from the parent semaphore_t when it is destroyed
    void invalidate();

    void add_wait(wait_callback_t *cb);
    void remove_wait(wait_callback_t *cb);

    semaphore_t *m_parent;
    size_t m_owned;
    size_t m_pending;
    intrusive_list_t<wait_callback_t> m_waiters;
};

class semaphore_t {
public:
    semaphore_t(semaphore_t &&other);
    semaphore_t(size_t _capacity);
    ~semaphore_t();

    size_t capacity() const;
    size_t available() const;

    void extend(size_t count); // Increases the maximum size
    void remove(semaphore_acq_t &&destroy); // Decreases the maximum size

    semaphore_acq_t start_acq(size_t count);

private:
    friend class semaphore_acq_t;
    void pump_waiters();

    size_t m_capacity;
    size_t m_available;
    intrusive_list_t<semaphore_acq_t> m_acqs;
    intrusive_list_t<semaphore_acq_t> m_pending;
};

} // namespace indecorous

#endif // SYNC_SEMAPHORE_HPP_
