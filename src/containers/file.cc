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
        eintr_wrap([&] { return close(m_fd); });
    }
}

fd_t scoped_fd_t::get() const {
    return m_fd;
}

bool scoped_fd_t::valid() const {
    return m_fd != s_invalid_fd;
}

} // namespace indecorous
