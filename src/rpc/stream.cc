#include "rpc/stream.hpp"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>

#include "rpc/message.hpp"
#include "sync/file_wait.hpp"
#include "sync/multiple_wait.hpp"
#include "utils.hpp"

namespace indecorous {

stream_t::~stream_t() { }

local_stream_t::local_stream_t() :
        m_fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
        m_queue() {
    assert(m_fd.valid());
}

void local_stream_t::write(write_message_t &&msg) {
    m_queue.push(std::move(msg).release().release());

    // TODO: would be nice to batch these, reduce system call overhead
    // don't want to increase latency, though =(
    uint64_t value = 1;
    GUARANTEE_ERR(::write(m_fd.get(), &value, sizeof(value)) == sizeof(value));
}

read_message_t local_stream_t::read() {
    buffer_owner_t buffer = buffer_owner_t::from_heap(m_queue.pop());

    if (buffer.has()) {
        return read_message_t::parse(std::move(buffer));
    }

    return read_message_t::empty();
}

void local_stream_t::wait() {
    // TODO: this involves a TLS-lookup, but it's only used from a place that
    // already has the TLS value.
    file_wait_t::in(m_fd.get()).wait();

    // Clear the eventfd now - this may result in a spurious wakeup later, but
    // better than missing a message.
    uint64_t value;
    auto res = eintr_wrap([&] { return ::read(m_fd.get(), &value, sizeof(value)); });
    if (res != sizeof(value)) {
        GUARANTEE_ERR(errno == EAGAIN || errno == EWOULDBLOCK);
    }
    assert(value > 0);
}

size_t local_stream_t::size() const {
    return m_queue.size();
}

io_stream_t::io_stream_t() :
    m_fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE)),
    m_read_lock(),
    m_queue() {
}

void io_stream_t::wait() {
    // Do nothing - `read` does the waiting
}

void io_stream_t::write(write_message_t &&msg) {
    m_queue.push(std::move(msg).release().release());
    uint64_t value = 1;
    auto res = eintr_wrap([&] { return ::write(m_fd.get(), &value, sizeof(value)); });
    GUARANTEE_ERR(res == sizeof(value));
}

read_message_t io_stream_t::read() {
    // TODO: this involves a TLS-lookup, but it's only used from a place that
    // already has the TLS value.
    file_wait_t in = file_wait_t::in(m_fd.get());
    uint64_t value;

    while (true) {
        in.wait();

        auto res = eintr_wrap([&] { return ::read(m_fd.get(), &value, sizeof(value)); });
        if (res == sizeof(value)) {
            GUARANTEE(value == 1);
            break;
        }
        GUARANTEE_ERR(errno == EAGAIN || errno == EWOULDBLOCK);
    }

    linkable_buffer_t *raw_buffer;
    {
        spinlock_acq_t lock(&m_read_lock);
        raw_buffer = m_queue.pop();
    }

    GUARANTEE(raw_buffer != nullptr);
    return read_message_t::parse(buffer_owner_t::from_heap(raw_buffer));
}

size_t io_stream_t::size() const {
    return m_queue.size();
}

tcp_stream_t::tcp_stream_t(int fd) : m_fd(fd) { }

read_message_t tcp_stream_t::read() {
    return read_message_t::parse(this);
}

void tcp_stream_t::write(write_message_t &&msg) {
    buffer_owner_t buffer = std::move(msg).release();
    write_exactly(buffer.data(), buffer.capacity());
}

void tcp_stream_t::wait() {
    // TODO: implement;
}

void tcp_stream_t::read_exactly(char *, size_t) {
    // TODO: implement
}

void tcp_stream_t::write_exactly(char *, size_t) {
    // TODO: implement
}

} // namespace indecorous
