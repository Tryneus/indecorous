#ifndef SYNC_TIMER_HPP_
#define SYNC_TIMER_HPP_

#include <cstdint>

#include "common.hpp"
#include "containers/intrusive.hpp"
#include "sync/wait_object.hpp"

namespace indecorous {

class events_t;

class absolute_time_t {
public:
    // This will round up to the nearest millisecond
    static int64_t ms_diff(const absolute_time_t &a, const absolute_time_t &b);

    absolute_time_t();
    explicit absolute_time_t(int64_t delta_ms);
    absolute_time_t(const absolute_time_t &other);

    // Add delta_ms to the absolute time until it is in the future
    void update_periodic(int64_t delta_ms);

    absolute_time_t &operator = (const absolute_time_t &other);
    bool operator < (const absolute_time_t &other) const;
private:
    void add(int64_t delta_ms);
    uint64_t sec;
    uint64_t nsec;
};

class timer_callback_t : public intrusive_node_t<timer_callback_t> {
public:
    timer_callback_t();
    timer_callback_t(timer_callback_t &&other);

    void update(int64_t delta_ms);
    const absolute_time_t &timeout() const;
    virtual void timer_callback(wait_result_t result) = 0;
protected:
    absolute_time_t m_timeout;
private:
    DISABLE_COPYING(timer_callback_t);
};

class single_timer_t final : public waitable_t, private timer_callback_t {
public:
    single_timer_t();
    explicit single_timer_t(int64_t timeout_ms); // Starts the timer immediately
    single_timer_t(single_timer_t &&other);
    ~single_timer_t();

    void start(int64_t timeout_ms);
    void stop();

private:
    void add_wait(wait_callback_t* cb) override final;
    void remove_wait(wait_callback_t* cb) override final;
    void timer_callback(wait_result_t result) override final;

    bool m_triggered;
    intrusive_list_t<wait_callback_t> m_waiters;
    events_t *m_thread_events;

    DISABLE_COPYING(single_timer_t);
};

class periodic_timer_t final : public waitable_t, private timer_callback_t {
public:
    periodic_timer_t();
    explicit periodic_timer_t(int64_t period_ms); // Starts the timer immediately
    periodic_timer_t(periodic_timer_t &&other);
    ~periodic_timer_t();

    void start(int64_t period_ms);
    void stop();

private:
    void add_wait(wait_callback_t* cb) override final;
    void remove_wait(wait_callback_t* cb) override final;
    void timer_callback(wait_result_t result) override final;

    void stop_internal(wait_result_t result);

    int64_t m_period_ms;
    intrusive_list_t<wait_callback_t> m_waiters;
    events_t *m_thread_events;

    DISABLE_COPYING(periodic_timer_t);
};

} // namespace indecorous

#endif
