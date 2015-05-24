#ifndef SYNC_TIMER_HPP_
#define SYNC_TIMER_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class timer_t : public wait_object_t, public wait_callback_t {
public:
    timer_t();
    ~timer_t();

    void set(uint32_t timeoutMs, bool autoReset, bool wakeAll);
    void reset();
    bool stop();
    void wait();

private:
    void addWait(wait_callback_t* cb);
    void removeWait(wait_callback_t* cb);

    friend class scheduler_t;
    void wait_callback(wait_result_t result);

    bool m_wakeAll;
    bool m_autoReset;
    bool m_running;
    uint32_t m_timeout;

    intrusive_list_t<wait_callback_t> m_waiters;
};

}; // namespace indecorous

#endif
