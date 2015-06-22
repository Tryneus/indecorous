#include "sync/file_wait.hpp"

#include "common.hpp"
#include "coro/coro.hpp"
#include "coro/events.hpp"
#include "coro/thread.hpp"

namespace indecorous {

file_callback_t::file_callback_t(int _fd, uint32_t _event_mask) :
    m_fd(_fd), m_event_mask(_event_mask) { }

file_callback_t::file_callback_t(file_callback_t &&other) :
        m_fd(other.m_fd), m_event_mask(other.m_event_mask) {
    other.m_fd = -1;
    other.m_event_mask = 0;
}

int file_callback_t::fd() const {
    return m_fd;
}

uint32_t file_callback_t::event_mask() const {
    return m_event_mask;
}

file_wait_t::file_wait_t(int _fd, uint32_t _event_mask) :
        file_callback_t(_fd, _event_mask),
        m_thread_events(thread_t::self()->events()) { }

file_wait_t::file_wait_t(file_wait_t &&other) :
        file_callback_t(std::move(other)),
        m_waiters(std::move(other.m_waiters)),
        m_thread_events(other.m_thread_events) {
    other.m_thread_events = nullptr;
}

file_wait_t file_wait_t::in(int fd) {
    return file_wait_t(fd, EPOLLIN);
}

file_wait_t file_wait_t::out(int fd) {
    return file_wait_t(fd, EPOLLOUT);
}

file_wait_t file_wait_t::err(int fd) {
    return file_wait_t(fd, EPOLLERR);
}

file_wait_t file_wait_t::hup(int fd) {
    return file_wait_t(fd, EPOLLHUP);
}

file_wait_t file_wait_t::pri(int fd) {
    return file_wait_t(fd, EPOLLPRI);
}

file_wait_t file_wait_t::rdhup(int fd) {
    return file_wait_t(fd, EPOLLRDHUP);
}

void file_wait_t::file_callback(wait_result_t result) {
    m_waiters.clear([&] (wait_callback_t *cb) { cb->wait_done(result); });
}

void file_wait_t::add_wait(wait_callback_t* cb) {
    if (m_waiters.empty()) {
        m_thread_events->add_file_wait(this);
    }
    m_waiters.push_back(cb);
}

void file_wait_t::remove_wait(wait_callback_t* cb) {
    m_waiters.remove(cb);
    if (m_waiters.empty()) {
        m_thread_events->remove_file_wait(this);
    }
}

} // namespace indecorous
