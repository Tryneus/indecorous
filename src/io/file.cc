#include "io/file.hpp"

#include <sys/eventfd.h>
#include <unistd.h>

#include "coro/coro.hpp"
#include "sync/file_wait.hpp"
#include "sync/interruptor.hpp"

namespace indecorous {

file_t::file_t(std::string filename, int flags, int permissions) :
        m_filename(std::move(filename)),
        m_flags(flags),
        m_file([&] {
                // TODO: mix in flags like non-block/cloexec?
                scoped_fd_t res(::open(m_filename.c_str(), flags, permissions));
                GUARANTEE_ERR(res.valid());
                debugf("opened file descriptor: %d", res.get());
                return res;
            }()),
        m_eventfd([&] {
                scoped_fd_t res(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
                GUARANTEE_ERR(res.valid());
                debugf("eventfd file descriptor: %d", res.get());
                return res;
            }()),
        m_extant_requests(),
        m_completed_requests(),
        m_drainer() {
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

void print_acb(const aiocb *acb) {
    debugf("acb.aio_fildes = %d", acb->aio_fildes);
    debugf("acb.aio_offset = %zu", acb->aio_offset);
    debugf("acb.aio_buf = %p", acb->aio_buf);
    debugf("acb.aio_nbytes = %zu", acb->aio_nbytes);
    debugf("acb.aio_reqprio = %d", acb->aio_reqprio);
    debugf("acb.aio_lio_opcode = %d", acb->aio_lio_opcode);
    debugf("acb.aio_sigevent.sigev_notify = %d", acb->aio_sigevent.sigev_notify);
    debugf("acb.aio_sigevent.sigev_signo = %d", acb->aio_sigevent.sigev_signo);
    debugf("acb.aio_sigevent.sigev_value.sival_ptr = %p", acb->aio_sigevent.sigev_value.sival_ptr);
    debugf("acb.aio_sigevent.sigev_notify_function = %p", acb->aio_sigevent.sigev_notify_function);
    debugf("acb.aio_sigevent.sigev_notify_attributes = %p", acb->aio_sigevent.sigev_notify_attributes);
}

future_t<int> file_t::write(uint64_t offset, const void *buffer, size_t size) {
    request_t *req = new request_t(this, offset, const_cast<void *>(buffer), size);
    future_t<int> res = req->m_promise.get_future();

    print_acb(&req->m_acb);

    if (aio_write(&req->m_acb) != 0) {
        auto errno_sv = errno;
        debugf("aio_write failed: %s (%d)", strerror(errno_sv), errno_sv);
        req->m_promise.fulfill(errno_sv);
        delete req;
    } else {
        debugf("aio_write success");
        m_extant_requests.insert(req);
    }

    return res;
}

future_t<int> file_t::read(uint64_t offset, void *buffer, size_t size) {
    request_t *req = new request_t(this, offset, buffer, size);
    future_t<int> res = req->m_promise.get_future();

    print_acb(&req->m_acb);

    if (aio_read(&req->m_acb) != 0) {
        auto errno_sv = errno;
        debugf("aio_read failed: %s (%d)", strerror(errno_sv), errno_sv);
        req->m_promise.fulfill(errno_sv);
        delete req;
    } else {
        debugf("aio_read success");
        m_extant_requests.insert(req);
    }

    return res;
}

// This callback is called from an arbitrary thread (and may even be called
// simultaneously from multiple threads - for different requests), so we need
// to be very careful about race conditions.
void file_t::done_callback(sigval_t val) {
    debugf("got completion for aio operation (%p)", val.sival_ptr);
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
        debugf("got response for aio request");
        int res = aio_error(&req->m_acb);
        if (res == 0) {
            ssize_t size = aio_return(&req->m_acb);
            GUARANTEE(static_cast<size_t>(size) == req->m_acb.aio_nbytes);
        }
        debugf("aio_error: %s (%d)", strerror(res), res);
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
    memset(&m_acb, 0, sizeof(m_acb));
    m_acb.aio_fildes = m_parent->m_file.get();
    m_acb.aio_offset = offset;
    m_acb.aio_buf = buffer;
    m_acb.aio_nbytes = size;
    m_acb.aio_sigevent.sigev_notify = SIGEV_THREAD;
    m_acb.aio_sigevent.sigev_value.sival_ptr = this;
    m_acb.aio_sigevent.sigev_notify_function = &file_t::done_callback;
}

} // namespace indecorous
