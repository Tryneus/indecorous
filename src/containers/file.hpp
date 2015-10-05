#ifndef CONTAINERS_FILE_HPP_
#define CONTAINERS_FILE_HPP_

#include "common.hpp"

namespace indecorous {

class scoped_fd_t {
public:
    explicit scoped_fd_t(fd_t fd);
    scoped_fd_t(scoped_fd_t &&other);

    ~scoped_fd_t();

    fd_t get() const;
    bool valid() const;

private:
    static const fd_t s_invalid_fd = -1;
    fd_t m_fd;

    DISABLE_COPYING(scoped_fd_t);
};

} // namespace indecorous

#endif // CONTAINERS_FILE_HPP_
