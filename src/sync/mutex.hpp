#ifndef SYNC_MUTEX_HPP_
#define SYNC_MUTEX_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class mutex_t : public wait_object_t {
public:
    mutex_t();
    ~mutex_t();

    // Successfully waiting on a mutex locks it
    void unlock();

private:
    void add_wait(wait_callback_t* cb);
    void remove_wait(wait_callback_t* cb);

    bool m_locked;
    intrusive_list_t<wait_callback_t> m_waiters;
};

class mutex_lock_t {
public:
    mutex_lock_t(mutex_t *parent);
    ~mutex_lock_t();
private:
    mutex_t *m_parent;
};

} // namespace indecorous

#endif // SYNC_MUTEX_HPP_
