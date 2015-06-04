#ifndef SYNC_SEMAPHORE_HPP_
#define SYNC_SEMAPHORE_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class semaphore_t : public wait_object_t {
public:
    semaphore_t(size_t initial, size_t max);
    ~semaphore_t();

    void unlock(size_t count);

private:
    void add_wait(wait_callback_t* cb);
    void remove_wait(wait_callback_t* cb);

    const size_t m_max;
    size_t m_count;
    intrusive_list_t<wait_callback_t> m_waiters;
};

} // namespace indecorous

#endif
