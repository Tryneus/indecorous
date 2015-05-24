#ifndef SYNC_FILE_WAIT_HPP_
#define SYNC_FILE_WAIT_HPP_
#include <poll.h>

#include "sync/wait_object.hpp"
#include "containers/intrusive.hpp"
#include "coro/coro.hpp"

namespace indecorous {

class file_wait_base_t : public wait_object_t, public wait_callback_t {
public:
    file_wait_base_t(int fd, bool wakeAll);
    virtual ~file_wait_base_t();

protected:
    int m_fd;
    bool m_triggered;

    void addWait(wait_callback_t* cb);
    void removeWait(wait_callback_t* cb);

private:
    friend class scheduler_t;
    void wait_callback(wait_result_t result);

    bool m_wakeAll;
    intrusive_list_t<wait_callback_t> m_waiters;
};

template <int EventFlag>
class file_wait_template_t : public file_wait_base_t {
public:
    file_wait_template_t(int fd, bool wakeAll) : file_wait_base_t(fd, wakeAll) { }
    ~file_wait_template_t() { }

    void wait();
};

typedef file_wait_template_t<POLLIN> file_wait_in_t;
typedef file_wait_template_t<POLLOUT> file_wait_out_t;
typedef file_wait_template_t<POLLERR> file_wait_err_t; // TODO: merge with HUP?
typedef file_wait_template_t<POLLHUP> file_wait_hup_t;
typedef file_wait_template_t<POLLPRI> file_wait_pri_t;
typedef file_wait_template_t<POLLRDHUP> file_wait_rdhup_t;

} // namespace indecorous

#endif
