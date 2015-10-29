#include "coro/events.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <sys/epoll.h>

#include "sync/file_wait.hpp"
#include "sync/timer.hpp"

namespace indecorous {

events_t::file_info_t::file_info_t() :
        m_callbacks(), m_last_used_events(0) { }

events_t::file_info_t::file_info_t(file_info_t &&other) :
        m_callbacks(std::move(other.m_callbacks)),
        m_last_used_events(other.m_last_used_events) {
    other.m_last_used_events = 0;
}

events_t::events_t() :
        m_timer_list(),
        m_epoll_changes(),
        m_file_map(),
        m_epoll_set(::epoll_create1(EPOLL_CLOEXEC)) {
    assert(m_epoll_set.valid());
}

events_t::~events_t() {

}

void events_t::add_timer(timer_callback_t *cb) {
    // TODO: make an intrusive tree and use binary search?
    timer_callback_t *cursor = m_timer_list.front();
    while (cursor != nullptr && cursor->timeout() < cb->timeout()) {
        cursor = m_timer_list.next(cursor);
    }

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
    assert(cb->event_mask() != 0);
    auto it = m_file_map.find(cb->fd());
    if (it == m_file_map.end()) {
        auto res = m_file_map.emplace(cb->fd(), file_info_t());
        assert(res.second);
        it = res.first;
    }
    it->second.m_callbacks.push_back(cb);
    m_epoll_changes.insert(cb->fd());
}

void events_t::remove_file_wait(file_callback_t *cb) {
    auto it = m_file_map.find(cb->fd());
    assert(it != m_file_map.end());
    it->second.m_callbacks.remove(cb);
    m_epoll_changes.insert(cb->fd());
}

void events_t::check(bool wait) {
    int timeout = 0;
    if (wait) {
        absolute_time_t start_time(0);
        timeout = -1;
        if (!m_timer_list.empty()) {
            timeout = absolute_time_t::ms_diff(m_timer_list.front()->timeout(), start_time);

            if (timeout < 0) {
                timeout = 0;
            }
        }
    }

    update_epoll();
    do_epoll_wait(timeout);

    absolute_time_t end_time(0);
    while (!m_timer_list.empty() && m_timer_list.front()->timeout() < end_time) {
        timer_callback_t *cb = m_timer_list.pop_front();
        cb->timer_callback(wait_result_t::Success);
    }
}

void events_t::update_epoll() {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));

    for (int fd : m_epoll_changes) {
        auto it = m_file_map.find(fd);
        assert(it != m_file_map.end());

        event.events = 0;
        event.data.fd = fd;
        file_callback_t *cb = it->second.m_callbacks.front();
        while (cb != nullptr) {
            event.events |= cb->event_mask();
            assert(cb->event_mask() != 0);
            cb = it->second.m_callbacks.next(cb);
        }

        if (event.events == 0) {
            assert(it->second.m_callbacks.empty());
            m_file_map.erase(it);

            // If the fd was closed, it may have been automatically removed from the set
            int res = ::epoll_ctl(m_epoll_set.get(), EPOLL_CTL_DEL, fd, &event);
            GUARANTEE_ERR(res == 0 || errno == EBADF || errno == ENOENT);
        } else {
            int task = it->second.m_last_used_events == 0 ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
            it->second.m_last_used_events = event.events;
            GUARANTEE_ERR(::epoll_ctl(m_epoll_set.get(), task, fd, &event) == 0);
        }
    }
    m_epoll_changes.clear();
}

void events_t::do_epoll_wait(int timeout) {
    size_t events_size = std::max<size_t>(m_file_map.size(), 1);
    std::unique_ptr<epoll_event[]> events(new epoll_event[events_size]);
    int res = ::epoll_wait(m_epoll_set.get(), events.get(), events_size, timeout);
    if (res <= 0) {
        // Ignore EINTR, just allow a spurious wakeup
        assert(res == 0 || errno == EINTR);
        return;
    }

    size_t count = res;
    assert(count <= m_file_map.size());
    for (size_t i = 0; i < count; ++i) {
        int fd = events[i].data.fd;
        uint32_t event_mask = events[i].events;

        auto it = m_file_map.find(fd);
        assert(it != m_file_map.end());

        intrusive_list_t<file_callback_t> *cbs = &it->second.m_callbacks;
        file_callback_t *cursor = cbs->front();

        while (cursor != nullptr) {
            file_callback_t *next = cbs->next(cursor);
            if (event_mask & (EPOLLERR | EPOLLHUP)) {
                cursor->file_callback(wait_result_t::ObjectLost); // TODO: better error type for this?
                cbs->remove(cursor);
            } else if ((cursor->event_mask() & event_mask) != 0) {
                cursor->file_callback(wait_result_t::Success);
                cbs->remove(cursor);
            }
            cursor = next;
        }
    }
}

} // namespace indecorous
