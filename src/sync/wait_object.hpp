#ifndef SYNC_WAIT_OBJECT_HPP_
#define SYNC_WAIT_OBJECT_HPP_

#include "containers/intrusive.hpp"

namespace indecorous {

class waitable_t;

enum class wait_result_t {
    Success,
    Interrupted,
    ObjectLost,
};

const char *wait_result_str(wait_result_t res);
void check_wait_result(wait_result_t result);

class wait_callback_t : public intrusive_node_t<wait_callback_t> {
public:
    wait_callback_t() = default;
    wait_callback_t(wait_callback_t &&other) = default;

    virtual void wait_done(wait_result_t result) = 0;
    virtual void object_moved(waitable_t *new_ptr) = 0;
};

class waitable_t {
public:
    virtual ~waitable_t() { }

    virtual void add_wait(wait_callback_t* waiter) = 0;
    virtual void remove_wait(wait_callback_t* waiter) = 0;

    void wait();
};

} // namespace indecorous

#endif // SYNC_WAIT_OBJECT_HPP_
