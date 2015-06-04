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

void check_wait_result(wait_result_t result);

class wait_callback_t : public intrusive_node_t<wait_callback_t> {
public:
    wait_callback_t() = default;
    wait_callback_t(wait_callback_t &&other) = default;

    virtual void wait_done(wait_result_t result) = 0;
};

class wait_object_t {
public:
    virtual ~wait_object_t() { }

    void wait();
    virtual void add_wait(wait_callback_t* waiter) = 0;
    virtual void remove_wait(wait_callback_t* waiter) = 0;
};

} // namespace indecorous

#endif // SYNC_WAIT_OBJECT_HPP_
