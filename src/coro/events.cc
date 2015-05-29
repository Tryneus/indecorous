#include "coro/events.hpp"

#include <algorithm>

#include "sync/file_wait.hpp"
#include "sync/timer.hpp"

namespace indecorous {

events_t::file_info_t::file_info_t() : last_used_events(0) { }

events_t::file_info_t::file_info_t(file_info_t &&other) :
        callbacks(std::move(other.callbacks)), last_used_events(other.last_used_events) {
    other.last_used_events = 0;
}

events_t::events_t() : m_epoll_set(epoll_create1(EPOLL_CLOEXEC)) {
    assert(m_epoll_set.valid());
}

events_t::~events_t() {

}

void events_t::add_timer(timer_callback_t *cb) {
    timer_callback_t *cursor = m_timer_list.front();
    while (cursor != nullptr && cursor->timeout() < cb->timeout()) { }

    if (cursor == nullptr) {
        m_timer_list.push_back(cb);
    } else {
        m_timer_list.insert_before(cursor, cb);
    }
}

void events_t::remove_timer(timer_callback_t *cb) {
    m_timer_list.remove(cb);
}

void events_t::add_file_wait(file_callback_t *cb) {
    auto it = m_file_map.find(cb->fd());
    if (it == m_file_map.end()) {
        auto res = m_file_map.emplace(cb->fd(), file_info_t());
        assert(res.second);
        it = res.first;
    }
    it->second.callbacks.push_back(cb);
    m_epoll_changes.insert(cb->fd());
}

void events_t::remove_file_wait(file_callback_t *cb) {
    auto it = m_file_map.find(cb->fd());
    assert(it != m_file_map.end());
    it->second.callbacks.remove(cb);
    m_epoll_changes.insert(cb->fd());
}

void events_t::wait() {
    absolute_time_t start_time(0);
    int timeout = -1;
    if (!m_timer_list.empty()) {
        timeout = absolute_time_t::ms_diff(start_time, m_timer_list.front()->timeout());
    }

    update_epoll();
    do_epoll_wait(timeout);
    
    absolute_time_t end_time(0);
    while (!m_timer_list.empty() && m_timer_list.front()->timeout() < end_time) {
        m_timer_list.pop_front()->timer_callback(wait_result_t::Success);
    }
}

void events_t::update_epoll() {
    for (int fd : m_epoll_changes) {
        auto it = m_file_map.find(fd);
        assert(it != m_file_map.end());

        epoll_event event;
        event.data.fd = fd;
        file_callback_t *cb = it->second.callbacks.front();
        while (cb != nullptr) {
            event.events |= cb->event_mask();
            cb = it->second.callbacks.next(cb);
        }

        int task;
        if (event.events == 0) {
            assert(it->second.callbacks.empty());
            m_file_map.erase(it);
            task = EPOLL_CTL_DEL;
        } else {
            task = it->second.last_used_events == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
            it->second.last_used_events = event.events;
        }
        int res = epoll_ctl(m_epoll_set.get(), task, fd, &event);
        assert(res == 0);
    }
    m_epoll_changes.clear();
}

void events_t::do_epoll_wait(int timeout) {
    size_t events_size = std::max<size_t>(m_file_map.size(), 1);
    std::unique_ptr<epoll_event[]> events(new epoll_event[events_size]);
    int res = epoll_wait(m_epoll_set.get(), events.get(), events_size, timeout);
    if (res <= 0) {
        // Ignore EINTR, just allow a spurious wakeup
        assert(res == 0 || res == EINTR);
        return;
    }

    size_t count = res;
    assert(count <= m_file_map.size());
    for (size_t i = 0; i < count; ++i) {
        int fd = events[i].data.fd;
        uint32_t event_mask = events[i].events;

        auto it = m_file_map.find(fd);
        assert(it != m_file_map.end());

        file_callback_t *cursor = it->second.callbacks.front();

        while (cursor != nullptr) {
            if (event_mask & EPOLLERR) {
                cursor->file_callback(wait_result_t::ObjectLost); // TODO: better error type for this?
            } else if ((cursor->event_mask() & event_mask) != 0) {
                cursor->file_callback(wait_result_t::Success);
            }
            cursor = it->second.callbacks.next(cursor);
        }
    }
}

} // namespace indecorous
