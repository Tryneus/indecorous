#include "message.hpp"

#include <cassert>

#include "stream.hpp"

struct message_header_t {
    static const uint64_t HEADER_MAGIC = 0x302ca58d7f47e0be;
    uint64_t header_magic;
    uint64_t handler_id;
    uint64_t request_id;
    uint64_t payload_size;
    MAKE_SERIALIZABLE(message_header_t,
                      header_magic,
                      handler_id,
                      request_id,
                      payload_size);
};

read_message_t read_message_t::parse(stream_t *stream) {
    static_assert(sizeof(message_header_t) == sizeof(uint64_t) * 4, "error");

    std::vector<char> header_buffer;
    header_buffer.resize(sizeof(message_header_t));
    stream->read(header_buffer.data(), header_buffer.size());

    read_message_t header_message(std::move(header_buffer), handler_id_t(-1), request_id_t(-1));
    message_header_t header = serializer_t<message_header_t>::read(&header_message);
    assert(header.header_magic == message_header_t::HEADER_MAGIC);

    std::vector<char> body_buffer;
    body_buffer.resize(header.payload_size);
    stream->read(body_buffer.data(), body_buffer.size());

    return read_message_t(std::move(body_buffer),
                          std::move(header.handler_id),
                          std::move(header.request_id));
}

read_message_t::read_message_t(std::vector<char> &&_buffer,
                               handler_id_t &&_handler_id,
                               request_id_t &&_request_id) :
    buffer(std::move(_buffer)),
    offset(0),
    handler_id(std::move(_handler_id)),
    request_id(std::move(_request_id)) { }

char read_message_t::pop() {
    assert(offset < buffer.size());
    return buffer[offset++];
}

write_message_t::write_message_t(handler_id_t handler_id,
                                 request_id_t request_id,
                                 size_t payload_size) {
    message_header_t header(message_header_t::HEADER_MAGIC, handler_id.value(), request_id.value(), payload_size);
    buffer.reserve(serializer_t<message_header_t>::size(header) + payload_size);
    serializer_t<message_header_t>::write(this, std::move(header));
}

void write_message_t::push_back(char c) {
    assert(buffer.capacity() >= buffer.size() + 1);
    buffer.push_back(c);
}
