#include "rpc/stream.hpp"

#include <cassert>

#include "coro/thread.hpp"
#include "rpc/message.hpp"

namespace indecorous {

stream_t::~stream_t() { }

local_stream_t::local_stream_t(thread_t *_thread) :
    thread(_thread) { }

void local_stream_t::write(write_message_t &&msg) {
    message_queue.push(std::move(msg).release().release());
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

void tcp_stream_t::read_exactly(char *, size_t) {
    // TODO: implement
}

void tcp_stream_t::write_exactly(char *, size_t) {
    // TODO: implement
}

} // namespace indecorous
