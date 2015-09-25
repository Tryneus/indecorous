#ifndef SYNC_FILE_WAIT_HPP_
#define SYNC_FILE_WAIT_HPP_

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class events_t;

class file_callback_t : public intrusive_node_t<file_callback_t> {
public:
    file_callback_t(int _fd, uint32_t _event_mask);
    file_callback_t(file_callback_t &&other);

    int fd() const;
    uint32_t event_mask() const;

    virtual void file_callback(wait_result_t result) = 0;

private:
    int m_fd;
    uint32_t m_event_mask;

    DISABLE_COPYING(file_callback_t);
};

class file_wait_t final : public waitable_t, private file_callback_t {
public:
    file_wait_t(file_wait_t &&other);

    static file_wait_t in(int fd);
    static file_wait_t out(int fd);
    static file_wait_t err(int fd);
    static file_wait_t hup(int fd);
    static file_wait_t pri(int fd);
    static file_wait_t rdhup(int fd);

private:
    void add_wait(wait_callback_t* cb) override final;
    void remove_wait(wait_callback_t* cb) override final;

    file_wait_t(int _fd, uint32_t _event_mask);

    void file_callback(wait_result_t result);

    intrusive_list_t<wait_callback_t> m_waiters;
    events_t *m_thread_events;

    DISABLE_COPYING(file_wait_t);
};

} // namespace indecorous

#endif
