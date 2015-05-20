#ifndef SYNC_WAIT_OBJECT_HPP_
#define SYNC_WAIT_OBJECT_HPP_

#include "containers/queue.hpp"

namespace indecorous {

class wait_object_t;

enum class wait_result_t {
    Success,
    Error,
    Interrupted,
    ObjectLost,
};

class wait_callback_t : public intrusive_node_t<wait_callback_t> {
public:
    virtual void wait_callback(wait_result_t result) = 0;
};

class wait_object_t {
public:
    virtual ~wait_object_t() { };

    virtual void wait() = 0;
protected:
    friend class multiple_wait_t; // For waits performed across multiple wait objects:
    virtual void addWait(wait_callback_t* waiter) = 0;
    virtual void removeWait(wait_callback_t* waiter) = 0;
};

} // namespace indecorous

#endif // SYNC_WAIT_OBJECT_HPP_
