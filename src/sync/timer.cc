#include "sync/timer.hpp"

#include <time.h>

#include "coro/events.hpp"
#include "coro/thread.hpp"

namespace indecorous {

absolute_time_t::absolute_time_t() : sec(0), nsec(0) { }

absolute_time_t::absolute_time_t(uint32_t delta_ms) {
    struct timespec t;
    int res = clock_gettime(CLOCK_MONOTONIC, &t);
    assert(res == 0);
    sec = t.tv_sec + (delta_ms / 1000);
    nsec = t.tv_nsec + (delta_ms % 1000) * 1000000;
    if (nsec > 1000000000) {
        sec += 1;
        nsec -= 1000000000;
        assert(nsec < 1000000000);
    }
}

absolute_time_t::absolute_time_t(const absolute_time_t &other) :
    sec(other.sec), nsec(other.nsec) { }

absolute_time_t &absolute_time_t::operator = (const absolute_time_t &other) {
    sec = other.sec;
    nsec = other.nsec;
    return *this;
}

int64_t absolute_time_t::ms_diff(const absolute_time_t &a, const absolute_time_t &b) {
    int64_t s_diff = a.sec - b.sec;
    int64_t ns_diff = a.nsec - b.nsec;
    return (s_diff * 1000) + (ns_diff / 1000);
}

bool absolute_time_t::operator < (const absolute_time_t &other) const {
    return (sec < other.sec) ? true : ((sec == other.sec) ? (nsec < other.nsec) : false);
}

timer_callback_t::timer_callback_t() { }

timer_callback_t::timer_callback_t(timer_callback_t &&other) :
    intrusive_node_t<timer_callback_t>(std::move(other)),
    m_timeout(other.m_timeout) { }

void timer_callback_t::update(uint32_t delta_ms) {
    m_timeout = absolute_time_t(delta_ms);
}

const absolute_time_t &timer_callback_t::timeout() const {
    return m_timeout;
}

single_timer_t::single_timer_t() :
    m_thread_events(thread_t::self()->events()) { }

single_timer_t::single_timer_t(single_timer_t &&other) :
        timer_callback_t(std::move(other)),
        m_triggered(false),
        m_waiters(std::move(other.m_waiters)),
        m_thread_events(other.m_thread_events) {
    other.m_thread_events = nullptr;
}

single_timer_t::~single_timer_t() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);   
    }
}

void single_timer_t::start(uint32_t timeout_ms) {
    m_triggered = false;
    update(timeout_ms);
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

void single_timer_t::wait() {
    if (!m_triggered) {
        assert(in_a_list()); // You cannot wait on a timer that is not running
        coro_wait(&m_waiters);
    }
}

void single_timer_t::addWait(wait_callback_t* cb) {
    if (m_triggered) {
        cb->wait_done(wait_result_t::Success);
    } else {
        assert(in_a_list());
        m_waiters.push_back(cb);
    }
}

void single_timer_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

void single_timer_t::timer_callback(wait_result_t result) {
    m_triggered = true;
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(result);       
    }
}

periodic_timer_t::periodic_timer_t(bool wake_one) :
    m_wake_one(wake_one),
    m_timeout_ms(0),
    m_thread_events(thread_t::self()->events()) { }

periodic_timer_t::periodic_timer_t(periodic_timer_t &&other) :
        timer_callback_t(std::move(other)),
        m_wake_one(other.m_wake_one),
        m_timeout_ms(other.m_timeout_ms),
        m_waiters(std::move(other.m_waiters)),
        m_thread_events(other.m_thread_events) {
    other.m_thread_events = nullptr;
}

periodic_timer_t::~periodic_timer_t() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);   
    }
}

void periodic_timer_t::start(uint32_t timeout_ms) {
    m_timeout_ms = timeout_ms;
    update(m_timeout_ms);
    if (in_a_list()) {
        // Re-`start`ing an already-running timer will update its timeout
        // without doing anything to current waiters
        m_thread_events->remove_timer(this);
    }
    m_thread_events->add_timer(this);
}

void periodic_timer_t::stop() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::Interrupted);   
    }
    if (in_a_list()) {
        m_thread_events->remove_timer(this);
    }
}

void periodic_timer_t::wait() {
    assert(in_a_list()); // You cannot wait on a timer that is not running
    coro_wait(&m_waiters);
}

void periodic_timer_t::addWait(wait_callback_t* cb) {
    assert(in_a_list());
    m_waiters.push_back(cb);
}

void periodic_timer_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

void periodic_timer_t::timer_callback(wait_result_t result) {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(result);       
    }
    update(m_timeout_ms);
    m_thread_events->add_timer(this);
}

} // namespace indecorous
