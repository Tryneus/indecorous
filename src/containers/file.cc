#include "containers/file.hpp"

#include <unistd.h>

#include "utils.hpp"

namespace indecorous {

scoped_fd_t::scoped_fd_t(fd_t fd) :
    m_fd(fd) { }

scoped_fd_t::scoped_fd_t(scoped_fd_t &&other) :
        m_fd(other.m_fd) {
    other.m_fd = s_invalid_fd;
}

scoped_fd_t::~scoped_fd_t() {
    if (valid()) {
        int res = eintr_wrap([&] { return close(m_fd); });
        GUARANTEE_ERR(res == 0);
    }
}

fd_t scoped_fd_t::get() const {
    return m_fd;
}

bool scoped_fd_t::valid() const {
    return m_fd != s_invalid_fd;
}

fd_t scoped_fd_t::release() {
    fd_t res = m_fd;
    m_fd = s_invalid_fd;
    return res;
}

} // namespace indecorous
