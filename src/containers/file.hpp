#ifndef CONTAINERS_FILE_HPP_
#define CONTAINERS_FILE_HPP_

#include <unistd.h>

namespace indecorous {

class scoped_fd_t {
public:
    scoped_fd_t(int fd) :
            m_fd(fd) { }

    scoped_fd_t(scoped_fd_t &&other) :
            m_fd(other.m_fd) {
        other.m_fd = s_invalid_fd;
    }

    ~scoped_fd_t() {
        if (valid()) { close(m_fd); }
    }

    int get() const { return m_fd; };
    bool valid() const { return m_fd != s_invalid_fd; }

private:
    static const int s_invalid_fd = -1;
    int m_fd;
};

} // namespace indecorous

#endif // CONTAINERS_FILE_HPP_
