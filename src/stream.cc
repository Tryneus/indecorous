#include "stream.hpp"

#include <cassert>

#include "debug.hpp"
#include "message.hpp"

stream_t::~stream_t() { }

dummy_stream_t::dummy_stream_t() { }

void dummy_stream_t::read(char *buffer, size_t length) {
    assert(data.size() >= length);
    for (size_t i = 0; i < length; ++i) {
        *(buffer++) = data[i];
    }
    data.erase(data.begin(), data.begin() + length);
}

void dummy_stream_t::write(char *buffer, size_t length) {
    std::copy(buffer, buffer + length, std::back_inserter(data));
}

tcp_stream_t::tcp_stream_t(int _fd) : fd(_fd) { }

void tcp_stream_t::read(char *, size_t) {
    // TODO: implement
    printf("Unimplemented tcp_stream_t::read with fd: %d\n", fd);
}

void tcp_stream_t::write(char *, size_t) {
    // TODO: implement
    printf("Unimplemented tcp_stream_t::write with fd: %d\n", fd);
}
