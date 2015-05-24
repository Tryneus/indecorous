#include "sync/file_wait.hpp"

#include "common.hpp"
#include "coro/coro.hpp"
#include "coro/thread.hpp"

namespace indecorous {

// TODO: only allow one coroutine to wait on a file at once?
//  maybe get rid of wakeall, so we only allow one wakeup per scheduler loop
file_wait_base_t::file_wait_base_t(int fd, bool wakeAll) :
    m_fd(fd), m_triggered(false), m_wakeAll(wakeAll) { }

file_wait_base_t::~file_wait_base_t() {
    while (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_callback(wait_result_t::ObjectLost);
    }
}

void file_wait_base_t::wait_callback(wait_result_t result) {
    if (m_wakeAll) {
        while (!m_waiters.empty()) {
            m_waiters.pop_front()->wait_callback(result);
        }
    } else if (!m_waiters.empty()) {
        m_waiters.pop_front()->wait_callback(result);
    }
}

void file_wait_base_t::addWait(wait_callback_t* cb) {
    m_waiters.push_back(cb);
}

void file_wait_base_t::removeWait(wait_callback_t* cb) {
    m_waiters.remove(cb);
}

template <int EventFlag>
void file_wait_template_t<EventFlag>::wait() {
    DEBUG_ONLY(coro_t* self = coro_t::self());
    addWait(coro_t::self());
    thread_t::add_file_wait(m_fd, EventFlag, this);
    coro_t::wait();
    assert(self == coro_t::self());
}

template class file_wait_template_t<POLLIN>;
template class file_wait_template_t<POLLOUT>;
template class file_wait_template_t<POLLERR>;
template class file_wait_template_t<POLLHUP>;
template class file_wait_template_t<POLLPRI>;
template class file_wait_template_t<POLLRDHUP>;

} // namespace indecorous
