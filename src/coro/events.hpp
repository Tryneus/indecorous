#ifndef CORO_EVENTS_HPP_
#define CORO_EVENTS_HPP_

#include <sys/epoll.h>

#include <unordered_map>
#include <unordered_set>

#include "containers/file.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class shutdown_t;
class timer_callback_t;
class file_callback_t;

class events_t {
public:
    events_t();
    ~events_t();

    void add_timer(timer_callback_t *cb);
    void remove_timer(timer_callback_t *cb);

    void add_file_wait(file_callback_t *cb);
    void remove_file_wait(file_callback_t *cb);

    void check(bool wait);

private:
    void update_epoll();
    void do_epoll_wait(int timeout);

    // List of active timers ordered earliest to latest
    intrusive_list_t<timer_callback_t> m_timer_list;

    // Queued changed to the epoll set since the last wait
    std::unordered_set<int> m_epoll_changes;

    struct file_info_t {
    public:
        file_info_t();
        file_info_t(file_info_t &&other);

        intrusive_list_t<file_callback_t> m_callbacks;
        uint32_t m_last_used_events;
    private:
        DISABLE_COPYING(file_info_t);
    };
    std::unordered_map<int, file_info_t> m_file_map;

    scoped_fd_t m_epoll_set;
};

};

#endif // CORO_EVENTS_HPP_
