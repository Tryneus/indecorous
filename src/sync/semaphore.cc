#include "sync/semaphore.hpp"

#include "common.hpp"

namespace indecorous {

semaphore_t::semaphore_t(size_t initial, size_t max) :
        m_max(max),
        m_count(initial) {
    assert(m_count <= m_max);
}

semaphore_t::~semaphore_t() {
    // Fail any remaining waiters
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_done(wait_result_t::ObjectLost);
    }
}

// TODO: can only lock one point at a time - allow larger waits without looping (tons of context switching overhead)
void semaphore_t::wait() {
    if (m_count > 0) {
        assert(m_waiters.empty());
        --m_count;
    } else {
        coro_wait(&m_waiters);
    }
}

void semaphore_t::unlock(size_t count) {
    m_count += count;
    assert(m_max >= m_count);

    for (size_t i = 0; i < count && !m_waiters.empty(); ++i) {
        assert(m_count > 0);
        --m_count;
        m_waiters.pop_front()->wait_done(wait_result_t::Success);
    }
}

void semaphore_t::addWait(wait_callback_t* cb) {
    if (m_count > 0) {
        assert(m_waiters.empty());
        --m_count;
        cb->wait_done(wait_result_t::Success);
    } else {
        m_waiters.push_back(cb);
    }
}

void semaphore_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

} // namespace indecorous

