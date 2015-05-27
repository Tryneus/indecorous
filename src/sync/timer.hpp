#ifndef SYNC_TIMER_HPP_
#define SYNC_TIMER_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class events_t;

class absolute_time_t {
public:
    static int64_t ms_diff(const absolute_time_t &a, const absolute_time_t &b);

    absolute_time_t();
    absolute_time_t(uint32_t delta_ms);
    absolute_time_t(const absolute_time_t &other);

    absolute_time_t &operator = (const absolute_time_t &other);
    bool operator < (const absolute_time_t &other) const;
private:
    uint64_t sec;
    uint64_t nsec;
};

class timer_callback_t : public intrusive_node_t<timer_callback_t> {
public:
    timer_callback_t();
    timer_callback_t(timer_callback_t &&other);

    void update(uint32_t delta_ms);
    const absolute_time_t &timeout() const;
    virtual void timer_callback(wait_result_t result) = 0;
private:
    absolute_time_t m_timeout;
};

class single_timer_t : public wait_object_t, private timer_callback_t {
public:
    single_timer_t();
    single_timer_t(single_timer_t &&other);
    ~single_timer_t();

    void start(uint32_t timeout_ms);
    void stop();

    void wait();

private:
    void addWait(wait_callback_t* cb);
    void removeWait(wait_callback_t* cb);

    void timer_callback(wait_result_t result);

    bool m_triggered;
    intrusive_list_t<wait_callback_t> m_waiters;
    events_t *m_thread_events;
};

class periodic_timer_t : public wait_object_t, private timer_callback_t {
public:
    periodic_timer_t(bool wake_one = false);
    periodic_timer_t(periodic_timer_t &&other);
    ~periodic_timer_t();

    void start(uint32_t timeout_ms);
    void stop();

    void wait();

private:
    void addWait(wait_callback_t* cb);
    void removeWait(wait_callback_t* cb);

    void timer_callback(wait_result_t result);

    bool m_wake_one;
    uint32_t m_timeout_ms;
    intrusive_list_t<wait_callback_t> m_waiters;
    events_t *m_thread_events;
};

}; // namespace indecorous

#endif
