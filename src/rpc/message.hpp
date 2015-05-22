#ifndef RPC_MESSAGE_HPP_
#define RPC_MESSAGE_HPP_

#include <cstddef>
#include <vector>

#include "containers/buffer.hpp"
#include "rpc/id.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

class tcp_stream_t;

class write_message_t {
public:
    template <typename... Args>
    static write_message_t create(handler_id_t handler_id,
                                  request_id_t request_id,
                                  Args &&...args);
    write_message_t(write_message_t &&other) = default;

    void push_back(char c);

private:
    write_message_t(handler_id_t handler_id,
                    request_id_t request_id,
                    size_t payload_size);

    friend class local_stream_t;
    friend class tcp_stream_t;
    buffer_owner_t m_buffer;
    size_t m_usage;
};

class read_message_t {
public:
    static read_message_t parse(buffer_owner_t &&buffer);
    static read_message_t parse(tcp_stream_t *stream);
    static read_message_t empty();
    read_message_t(read_message_t &&other) = default;

    char pop();

    buffer_owner_t buffer;
    size_t offset;
    handler_id_t handler_id;
    request_id_t request_id;

private:
    read_message_t(buffer_owner_t &&_buffer,
                   handler_id_t &&_handler_id,
                   request_id_t &&_request_id);
};

template <typename... Args>
write_message_t write_message_t::create(handler_id_t handler_id,
                                        request_id_t request_id,
                                        Args &&...args) {
    write_message_t res(handler_id, request_id,
                        full_serialized_size(std::forward<Args>(args)...));
    full_serialize(&res, std::forward<Args>(args)...);
    return res;
}

} // namespace indecorous

#endif // MESSAGE_HPP_
