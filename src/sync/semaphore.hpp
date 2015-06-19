#ifndef SYNC_SEMAPHORE_HPP_
#define SYNC_SEMAPHORE_HPP_

#include <cstddef>

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class semaphore_t;

class semaphore_acq_t : public wait_object_t, public intrusive_node_t<semaphore_acq_t> {
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
    semaphore_t(size_t max);
    ~semaphore_t();

    void extend(size_t count); // Increases the maximum size
    void remove(semaphore_acq_t &&destroy); // Decreases the maximum size

    semaphore_acq_t acquire(size_t count);

private:
    friend class semaphore_acq_t;
    void pump_waiters();

    size_t m_max;
    size_t m_available;
    intrusive_list_t<semaphore_acq_t> m_acqs;
    intrusive_list_t<semaphore_acq_t> m_waiters;
};

} // namespace indecorous

#endif
