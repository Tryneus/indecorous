#include "io/file.hpp"

#include <sys/eventfd.h>
#include <sys/uio.h>
#include <unistd.h>

#include "coro/coro.hpp"
#include "rpc/handler.hpp"
#include "sync/file_wait.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

class file_callbacks_t {
    DECLARE_STATIC_RPC(read)(int fd, off_t offset, const void *buffer, size_t size) -> int;
    DECLARE_STATIC_RPC(write)(int fd, off_t offset, const void *buffer, size_t size) -> int;
};

file_t::file_t(std::string filename, int flags, int permissions) :
        m_filename(std::move(filename)),
        m_flags(flags),
        m_file([&] {
                // TODO: mix in flags like non-block/cloexec?
                scoped_fd_t res(::open(m_filename.c_str(), flags, permissions));
                GUARANTEE_ERR(res.valid());
                debugf("opened file descriptor: %d", res.get());
                return res;
            }()) { }

file_t::file_t(file_t &&other) :
        m_filename(std::move(other.m_filename)),
        m_flags(other.m_flags),
        m_file(std::move(other.m_file)) {
    other.m_flags = 0;
}

IMPL_STATIC_RPC(file_callbacks_t::read)
        (int fd, off_t offset, const void *buffer, size_t size) -> int {
    iovec iov[1];
    iov[0].iov_base = const_cast<void *>(buffer);
    iov[0].iov_len = size;
    ssize_t res = ::preadv(fd, &iov[0], 1, offset);
    while (res != size) {
        if (res == 0) {
            return errno;
        }
        buffer += res;
        size -= res;
        offset += size;
        iov[0].iov_base = buffer;
        iov[0].io_size = size;
        res = ::preadv(fd, &iov[0], 1, offset);
    }
    return 0;
}

IMPL_STATIC_RPC(file_callbacks_t::write)
        (int fd, off_t offset, const void *buffer, size_t size) -> int {
    iovec iov[1];
    iov[0].iov_base = buffer;
    iov[0].iov_len = size;
    ssize_t res = ::pwritev(fd, &iov[0], 1, offset);
    while (res != size) {
        if (res == 0) {
            return errno;
        }
        buffer += res;
        size -= res;
        offset += size;
        iov[0].iov_base = buffer;
        iov[0].io_size = size;
        res = ::pwritev(fd, &iov[0], 1, offset);
    }
}

future_t<int> file_t::write(off_t offset, const void *buffer, size_t size) {
    return thread_t::self()->hub()->io_pool()->
        call_async<file_callbacks_t::write>(m_file.get(), offset, buffer, size);
}

future_t<int> file_t::read(off_t offset, void *buffer, size_t size) {
    return thread_t::self()->hub()->io_pool()->
        call_async<file_callbacks_t::read>(m_file.get(), offset, buffer, size);
}

} // namespace indecorous
