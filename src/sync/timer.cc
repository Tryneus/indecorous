#include "sync/timer.hpp"

#include <time.h>

#include "coro/events.hpp"
#include "coro/thread.hpp"

namespace indecorous {

absolute_time_t::absolute_time_t() : sec(0), nsec(0) { }

absolute_time_t::absolute_time_t(int64_t delta_ms) {
    struct timespec t;
    GUARANTEE_ERR(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    sec = t.tv_sec;
    nsec = t.tv_nsec;
    add(delta_ms);
}

absolute_time_t::absolute_time_t(const absolute_time_t &other) :
    sec(other.sec), nsec(other.nsec) { }

absolute_time_t &absolute_time_t::operator = (const absolute_time_t &other) {
    sec = other.sec;
    nsec = other.nsec;
    return *this;
}

void absolute_time_t::update_periodic(int64_t delta_ms) {   
    absolute_time_t now(0);

    // TODO: this is slow for small delta_ms values, especially if a
    // large amount of time has passed in-between
    while (*this < now) {
        add(delta_ms);
    }
}

void absolute_time_t::add(int64_t delta_ms) {
    sec = sec + (delta_ms / 1000);
    nsec = nsec + (delta_ms % 1000) * 1000000;
    if (nsec > 1000000000) {
        sec += 1;
        nsec -= 1000000000;
        assert(nsec < 1000000000);
    }
}

int64_t absolute_time_t::ms_diff(const absolute_time_t &a, const absolute_time_t &b) {
    int64_t s_diff = a.sec - b.sec;
    int64_t ns_diff = a.nsec - b.nsec;
    return (s_diff * 1000) + (ns_diff / 1000000) + ((ns_diff % 1000000 > 0) ? 1 : 0);
}

bool absolute_time_t::operator < (const absolute_time_t &other) const {
    return (sec < other.sec) ? true : ((sec == other.sec) ? (nsec < other.nsec) : false);
}

timer_callback_t::timer_callback_t() { }

timer_callback_t::timer_callback_t(timer_callback_t &&other) :
    intrusive_node_t<timer_callback_t>(std::move(other)),
    m_timeout(other.m_timeout) { }

const absolute_time_t &timer_callback_t::timeout() const {
    return m_timeout;
}

single_timer_t::single_timer_t() :
    m_thread_events(thread_t::self()->events()) { }

single_timer_t::single_timer_t(int64_t timeout_ms) :
        m_thread_events(thread_t::self()->events()) {
    start(timeout_ms);
}

single_timer_t::single_timer_t(single_timer_t &&other) :
        timer_callback_t(std::move(other)),
        m_triggered(false),
        m_waiters(std::move(other.m_waiters)),
        m_thread_events(other.m_thread_events) {
    other.m_thread_events = nullptr;
    m_waiters.each([&] (wait_callback_t *w) { w->object_moved(this); });
}

single_timer_t::~single_timer_t() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);   
    }
}

void single_timer_t::start(int64_t timeout_ms) {
    m_triggered = false;
    m_timeout = absolute_time_t(timeout_ms);
    if (in_a_list()) {
        // Re-`start`ing an already-running timer will update its timeout
        // without doing anything to current waiters
        m_thread_events->remove_timer(this);
    }
    m_thread_events->add_timer(this);
}

void single_timer_t::stop() {
    m_triggered = false;
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::Interrupted);   
    }
    if (in_a_list()) {
        m_thread_events->remove_timer(this);
    }
}

void single_timer_t::add_wait(wait_callback_t* cb) {
    if (m_triggered) {
        cb->wait_done(wait_result_t::Success);
    } else {
        assert(in_a_list());
        m_waiters.push_back(cb);
    }
}

void single_timer_t::remove_wait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

void single_timer_t::timer_callback(wait_result_t result) {
    m_triggered = true;
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(result);       
    }
}

periodic_timer_t::periodic_timer_t() :
    m_period_ms(-1),
    m_thread_events(thread_t::self()->events()) { }

periodic_timer_t::periodic_timer_t(int64_t period_ms) :
        m_period_ms(-1),
        m_thread_events(thread_t::self()->events()) {
    start(period_ms);
}

periodic_timer_t::periodic_timer_t(periodic_timer_t &&other) :
        timer_callback_t(std::move(other)),
        m_period_ms(other.m_period_ms),
        m_waiters(std::move(other.m_waiters)),
        m_thread_events(other.m_thread_events) {
    other.m_thread_events = nullptr;
    m_waiters.each([&] (wait_callback_t *w) { w->object_moved(this); });
}

periodic_timer_t::~periodic_timer_t() {
    stop_internal(wait_result_t::ObjectLost);
}

void periodic_timer_t::stop_internal(wait_result_t result) {
    m_period_ms = -1;
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(result);
    }
    if (in_a_list()) {
        m_thread_events->remove_timer(this);
    }
}

void periodic_timer_t::start(int64_t period_ms) {
    m_period_ms = period_ms;
    m_timeout = absolute_time_t(m_period_ms);

    if (in_a_list()) {
        // Re-`start`ing an already-running timer will update its timeout
        // without doing anything to current waiters
        assert(m_waiters.empty());
        m_thread_events->remove_timer(this);
    }
    if (!m_waiters.empty()) {
        m_thread_events->add_timer(this);
    }
}

void periodic_timer_t::stop() {
    stop_internal(wait_result_t::Interrupted);
}

void periodic_timer_t::add_wait(wait_callback_t* cb) {
    m_waiters.push_back(cb);

    if (m_period_ms != -1) {
        if (!in_a_list()) {
            absolute_time_t now(0);
            m_timeout = absolute_time_t(m_period_ms);

            m_timeout.update_periodic(m_period_ms);
            m_thread_events->add_timer(this);
        }
    }
}

void periodic_timer_t::remove_wait(wait_callback_t* cb) {
    m_waiters.remove(cb);
    if (m_waiters.empty() && in_a_list()) {
        m_thread_events->remove_timer(this);
    }
}

void periodic_timer_t::timer_callback(wait_result_t result) {
    assert(m_period_ms != -1);
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(result);       
    }
}

} // namespace indecorous
