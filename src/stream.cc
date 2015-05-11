#include "include/stream.hpp"

#include <cassert>

#include "include/message.hpp"

void fake_stream_t::read(char *buffer, size_t length) {
    assert(data.size() >= length);
    for (size_t i = 0; i < length; ++i) {
        *(buffer++) = data[i];
    }
    data.erase(data.begin(), data.begin() + length);
}

void fake_stream_t::write(write_message_t &&msg) {
    std::copy(msg.buffer.begin(), msg.buffer.end(), std::back_inserter(data));
}
