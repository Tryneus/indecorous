#include "io/file.hpp"

#include <sys/eventfd.h>
#include <sys/uio.h>
#include <unistd.h>

#include "coro/coro.hpp"
#include "coro/thread.hpp"
#include "rpc/handler.hpp"
#include "rpc/serialize_stl.hpp"
#include "sync/file_wait.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

SERIALIZABLE_POINTER(void);

class file_callbacks_t {
public:
    DECLARE_STATIC_RPC(open)(std::string filename, int flags, int permissions) -> int;
    DECLARE_STATIC_RPC(read)(int fd, off_t offset, void *buffer, size_t size) -> int;
    DECLARE_STATIC_RPC(write)(int fd, off_t offset, void *buffer, size_t size) -> int;
};

IMPL_STATIC_RPC(file_callbacks_t::open)
        (std::string filename, int flags, int permissions) -> int {
    scoped_fd_t res(::open(filename.c_str(), flags, permissions));
    GUARANTEE_ERR(res.valid());
    return res.release();
}

IMPL_STATIC_RPC(file_callbacks_t::read)
        (int fd, off_t offset, void *buffer, size_t size) -> int {
    iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = size;
    ssize_t res = ::preadv(fd, &iov, 1, offset);
    while (static_cast<size_t>(res) != size) {
        if (res == 0) {
            return EINVAL; // TODO: we probably need to return the actual size of the read
        } else if (res == -1) {
            return errno;
        }
        offset += res;
        iov.iov_base = reinterpret_cast<char *>(iov.iov_base) + res;
        iov.iov_len -= res;
        res = ::preadv(fd, &iov, 1, offset);
    }
    return 0;
}

IMPL_STATIC_RPC(file_callbacks_t::write)
        (int fd, off_t offset, void *buffer, size_t size) -> int {
    iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = size;
    ssize_t res = ::pwritev(fd, &iov, 1, offset);
    while (static_cast<size_t>(res) != size) {
        if (res == -1) {
            return errno;
        }
        offset += res;
        iov.iov_base = reinterpret_cast<char *>(iov.iov_base) + res;
        iov.iov_len -= res;
        res = ::pwritev(fd, &iov, 1, offset);
    }
    return 0;
}

file_t::file_t(std::string filename, int flags, int permissions) :
        m_filename(std::move(filename)),
        m_flags(flags),
        m_file(thread_t::self()->hub()->io_target()->call_sync<file_callbacks_t::open>
                  (std::string(m_filename), std::move(flags), std::move(permissions))) // TODO: shouldn't need to move everything
{ }

file_t::file_t(file_t &&other) :
        m_filename(std::move(other.m_filename)),
        m_flags(other.m_flags),
        m_file(std::move(other.m_file)) {
    other.m_flags = 0;
}

future_t<int> file_t::write(off_t offset, const void *buffer, size_t size) {
    return thread_t::self()->hub()->io_target()->call_async<file_callbacks_t::write>
        (m_file.get(), std::move(offset), const_cast<void *>(buffer), std::move(size)); // TODO: shouldn't need to move everything
}

future_t<int> file_t::read(off_t offset, void *buffer, size_t size) {
    return thread_t::self()->hub()->io_target()->call_async<file_callbacks_t::read>
        (m_file.get(), std::move(offset), std::move(buffer), std::move(size)); // TODO: shouldn't need to move everything
}

} // namespace indecorous
