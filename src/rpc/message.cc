#include "rpc/message.hpp"

#include <cassert>

#include "rpc/stream.hpp"

namespace indecorous {

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

read_message_t::read_message_t(buffer_owner_t &&_buffer,
                               handler_id_t &&_handler_id,
                               request_id_t &&_request_id) :
    buffer(std::move(_buffer)),
    offset(0),
    handler_id(std::move(_handler_id)),
    request_id(std::move(_request_id)) { }

char read_message_t::pop() {
    assert(offset < buffer.capacity());
    return buffer.data()[offset++];
}

read_message_t read_message_t::empty() {
    return read_message_t(buffer_owner_t::empty(), handler_id_t(-1), request_id_t(-1));
}

read_message_t read_message_t::parse(buffer_owner_t &&buffer) {
    static_assert(sizeof(message_header_t) == sizeof(uint64_t) * 4, "error");
    read_message_t message(std::move(buffer), handler_id_t(-1), request_id_t(-1));
    message_header_t header = serializer_t<message_header_t>::read(&message);
    assert(header.header_magic == message_header_t::HEADER_MAGIC);

    message.handler_id = header.handler_id;
    message.request_id = header.request_id;

    return message;
}

read_message_t read_message_t::parse(tcp_stream_t *stream) {
    // Read the header into a stack-based array to save allocations
    std::array<char, sizeof(message_header_t) + sizeof(linkable_buffer_t)> array;
    buffer_owner_t header_buffer = buffer_owner_t::from_array(
        array.data(), array.size(), sizeof(message_header_t));
    stream->read_exactly(header_buffer.data(), header_buffer.capacity());

    read_message_t message(std::move(header_buffer), handler_id_t(-1), request_id_t(-1));
    message_header_t header = serializer_t<message_header_t>::read(&message);
    assert(header.header_magic == message_header_t::HEADER_MAGIC);

    buffer_owner_t body_buffer(header.payload_size);
    stream->read_exactly(body_buffer.data(), body_buffer.capacity());

    return read_message_t(std::move(body_buffer),
                          std::move(header.handler_id),
                          std::move(header.request_id));
}

write_message_t::write_message_t(handler_id_t handler_id,
                                 request_id_t request_id,
                                 size_t payload_size) :
        m_buffer(sizeof(message_header_t) + payload_size),
        m_usage(0) {
    message_header_t header(message_header_t::HEADER_MAGIC, handler_id.value(), request_id.value(), payload_size);
    assert(serializer_t<message_header_t>::size(header) == sizeof(message_header_t));
    serializer_t<message_header_t>::write(this, std::move(header));
}

void write_message_t::push_back(char c) {
    assert(m_buffer.capacity() >= m_usage + 1);
    m_buffer.data()[m_usage++] = c;
}

} // namespace indecorous
