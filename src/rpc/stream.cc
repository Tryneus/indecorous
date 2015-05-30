#include "rpc/stream.hpp"

#include <sys/eventfd.h>

#include <cassert>

#include "coro/thread.hpp"
#include "rpc/message.hpp"
#include "sync/file_wait.hpp"

namespace indecorous {

stream_t::~stream_t() { }

local_stream_t::local_stream_t(thread_t *_thread) :
        fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)), thread(_thread) {
    assert(fd.valid());
}

void local_stream_t::write(write_message_t &&msg) {
    message_queue.push(std::move(msg).release().release());

    // TODO: would be nice to batch these, reduce system call overhead
    // don't want to increase latency, though =(
    uint64_t value = 1;
    int res = ::write(fd.get(), &value, sizeof(value));
    assert(res == sizeof(value));
}

void local_stream_t::wait() {
    // TODO: this involves a TLS-lookup, but it's only used from a place that
    // already has the TLS value.
    file_wait_t::in(fd.get()).wait();

    // Clear the eventfd now - this may result in a spurious wakeup later, but
    // better than missing a message.
    uint64_t value;
    int res = ::read(fd.get(), &value, sizeof(value));
    assert(res == sizeof(value));
    assert(value > 0);
}

read_message_t local_stream_t::read() {
    assert(thread_t::self() == thread);
    buffer_owner_t buffer = buffer_owner_t::from_heap(message_queue.pop());

    if (buffer.has()) {
        return read_message_t::parse(std::move(buffer));
    }

    return read_message_t::empty();
}

tcp_stream_t::tcp_stream_t(int _fd) : fd(_fd) { }

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
