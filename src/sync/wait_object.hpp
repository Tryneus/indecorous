#ifndef SYNC_WAIT_OBJECT_HPP_
#define SYNC_WAIT_OBJECT_HPP_

#include "containers/intrusive.hpp"

namespace indecorous {

class wait_object_t;

enum class wait_result_t {
    Success,
    Interrupted,
    ObjectLost,
};

class wait_callback_t : public intrusive_node_t<wait_callback_t> {
public:
    virtual void wait_done(wait_result_t result) = 0;
};

class wait_object_t {
public:
    virtual ~wait_object_t() { }

protected:
    virtual void wait() = 0;
    virtual void addWait(wait_callback_t* waiter) = 0;
    virtual void removeWait(wait_callback_t* waiter) = 0;
};

// Adds the current coroutine to the list and waits
// This is useful to avoid circular dependencies with coro_t
void coro_wait(intrusive_list_t<wait_callback_t> *list);

} // namespace indecorous

#endif // SYNC_WAIT_OBJECT_HPP_
