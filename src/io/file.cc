#include "io/file.hpp"

#include <sys/eventfd.h>
#include <unistd.h>

#include "coro/coro.hpp"
#include "sync/file_wait.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

file_t::file_t(std::string filename, int flags) :
        m_filename(std::move(filename)),
        m_flags(flags),
        m_file([&] {
                // TODO: mix in flags like non-block/cloexec?
                scoped_fd_t res(::open(m_filename.c_str(), flags));
                GUARANTEE_ERR(res.valid());
                return res;
            }()),
        m_eventfd([&] {
                scoped_fd_t res(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
                GUARANTEE_ERR(res.valid());
                return res;
            }()) {
    // Coroutine that waits for completion of events and passes it back to waiting coroutines
    coro_t::spawn([&] (drainer_lock_t lock) {
            UNUSED interruptor_clear_t non_interruptor;
            interruptor_t drain(&lock);
            try {
                file_wait_t event = file_wait_t::in(m_eventfd.get());
                while (true) {
                    event.wait();
                    pull_results();
                }
            } catch (const wait_interrupted_exc_t &) {
                pull_results();
            }
        }, m_drainer.lock());
}

file_t::~file_t() {
    for (auto const &req : m_extant_requests) {
        // Doesn't matter if this is successful, we will still block
        // on the drainer until all requests have completed
        aio_cancel(m_file.get(), &req->m_acb);
    }
}

future_t<int> file_t::write(uint64_t offset, void *buffer, size_t size) {
    request_t *req = new request_t(this, offset, buffer, size);
    future_t<int> res = req->m_promise.get_future();

    if (aio_write(&req->m_acb) != 0) {
        req->m_promise.fulfill(errno);
    } else {
        m_extant_requests.insert(req);
    }

    return res;
}

future_t<int> file_t::read(uint64_t offset, void *buffer, size_t size) {
    request_t *req = new request_t(this, offset, buffer, size);
    future_t<int> res = req->m_promise.get_future();

    if (aio_read(&req->m_acb) != 0) {
        req->m_promise.fulfill(errno);
    } else {
        m_extant_requests.insert(req);
    }

    return res;
}

// This callback is called from an arbitrary thread (and may even be called
// simultaneously from multiple threads - for different requests), so we need
// to be very careful about race conditions.
void file_t::done_callback(sigval_t val) {
    request_t *request = reinterpret_cast<request_t *>(val.sival_ptr);
    fd_t parent_eventfd = request->m_parent->m_eventfd.get();

    request->m_parent->m_completed_requests.push(request);

    uint64_t buffer = 1;

    // The write could fail due to a race condition during file_t deletion,
    // but it should be easily recognizable.
    auto res = ::write(parent_eventfd, &buffer, sizeof(buffer));
    GUARANTEE_ERR(res == 0 || res == EBADF);
}

void file_t::pull_results() {
    while (auto req = m_completed_requests.pop()) {
        int res = aio_error(&req->m_acb);
        if (res == 0) {
            ssize_t size = aio_return(&req->m_acb);
            GUARANTEE(static_cast<size_t>(size) == req->m_acb.aio_nbytes);
        }
        req->m_promise.fulfill(res);
        m_extant_requests.erase(req);
    }
}

file_t::request_t::request_t(file_t *parent,
                             uint64_t offset,
                             void *buffer,
                             size_t size) :
        m_acb(),
        m_parent(parent),
        m_promise(),
        m_drainer_lock(parent->m_drainer.lock()) {
    m_acb.aio_fildes = m_parent->m_file.get();
    m_acb.aio_offset = offset;
    m_acb.aio_buf = buffer;
    m_acb.aio_nbytes = size;
}

} // namespace indecorous
