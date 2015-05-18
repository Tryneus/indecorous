#include "rpc/stream.hpp"

#include <cassert>

#include "rpc/message.hpp"

namespace indecorous {

stream_t::~stream_t() { }

local_stream_t::local_stream_t(thread_t *_thread) :
    thread(_thread) { }

void local_stream_t::write(write_message_t &&msg) {
    // TODO: somehow notify the thread that it has messages
    std::lock_guard<std::mutex> lock(mutex);
    message_queue.emplace(std::move(msg));
}

read_message_t local_stream_t::read() {
    std::vector<char> buffer;
    {
        std::lock_guard<std::mutex> lock(mutex);
        buffer.swap(message_queue.front().buffer);
        message_queue.pop();
    }
    return read_message_t::parse(std::move(buffer));
}

tcp_stream_t::tcp_stream_t(int _fd) : fd(_fd) { }

read_message_t tcp_stream_t::read() {
    // TODO: implement
    printf("Unimplemented tcp_stream_t::read with fd: %d\n", fd);
    return read_message_t::parse(std::vector<char>());
}

void tcp_stream_t::write(write_message_t &&) {
    // TODO: implement
    printf("Unimplemented tcp_stream_t::write with fd: %d\n", fd);
}

} // namespace indecorous
