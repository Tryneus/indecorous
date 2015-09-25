#ifndef CONTAINERS_FILE_HPP_
#define CONTAINERS_FILE_HPP_

#include <unistd.h>

#include "common.hpp"
#include "utils.hpp"

namespace indecorous {

typedef int fd_t;

class scoped_fd_t {
public:
    scoped_fd_t(fd_t fd) :
            m_fd(fd) { }

    scoped_fd_t(scoped_fd_t &&other) :
            m_fd(other.m_fd) {
        other.m_fd = s_invalid_fd;
    }

    ~scoped_fd_t() {
        if (valid()) {
            eintr_wrap([&] { return close(m_fd); });
        }
    }

    fd_t get() const { return m_fd; };
    bool valid() const { return m_fd != s_invalid_fd; }

private:
    static const fd_t s_invalid_fd = -1;
    fd_t m_fd;

    DISABLE_COPYING(scoped_fd_t);
};

} // namespace indecorous

#endif // CONTAINERS_FILE_HPP_
