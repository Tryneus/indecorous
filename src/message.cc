#include "message.hpp"

#include <cassert>

#include "stream.hpp"

struct message_header_t {
    static const uint64_t HEADER_MAGIC = 0x302ca58d7f47e0be;
    uint64_t header_magic;
    uint64_t handler_id;
    uint64_t request_id;
    uint64_t payload_size;
    DECLARE_SERIALIZABLE(message_header_t);
};

size_t message_header_t::serialized_size() const {
    return sizeof(message_header_t);
}

int message_header_t::serialize(write_message_t *msg) {
    ::serialize(msg, std::move(header_magic));
    ::serialize(msg, std::move(handler_id));
    ::serialize(msg, std::move(request_id));
    ::serialize(msg, std::move(payload_size));
    return 0;
}

message_header_t message_header_t::deserialize(read_message_t *msg) {
    return message_header_t({ ::deserialize<uint64_t>(msg),
                              ::deserialize<uint64_t>(msg),
                              ::deserialize<uint64_t>(msg),
                              ::deserialize<uint64_t>(msg) });
}

read_message_t read_message_t::parse(stream_t *stream) {
    static_assert(sizeof(message_header_t) == sizeof(uint64_t) * 4, "error");

    std::vector<char> header_buffer;
    header_buffer.resize(sizeof(message_header_t));
    stream->read(header_buffer.data(), header_buffer.size());

    read_message_t header_message(std::move(header_buffer), handler_id_t(-1), request_id_t(-1));
    message_header_t header = message_header_t::deserialize(&header_message);
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
    return buffer[offset++];
}

write_message_t::write_message_t(handler_id_t handler_id,
                                 request_id_t request_id,
                                 size_t payload_size) {
    message_header_t header({ message_header_t::HEADER_MAGIC, handler_id.value(), request_id.value(), payload_size });
    buffer.reserve(serialized_size(header) + payload_size);
    serialize(this, std::move(header));
}
