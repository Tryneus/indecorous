#ifndef RPC_MESSAGE_HPP_
#define RPC_MESSAGE_HPP_

#include <cstddef>
#include <vector>

#include "rpc/id.hpp"
#include "rpc/serialize.hpp"

namespace indecorous {

class stream_t;

class write_message_t {
public:
    template <typename... Args>
    static write_message_t create(handler_id_t handler_id,
                                  request_id_t request_id,
                                  Args &&...args);

    write_message_t(handler_id_t handler_id,
                    request_id_t request_id,
                    size_t payload_size);

    void push_back(char c);

private:
    friend class local_target_t;
    friend class remote_target_t;
    std::vector<char> buffer;
};

class read_message_t {
public:
    char pop();

    const std::vector<char> buffer;
    size_t offset;
    handler_id_t handler_id;
    request_id_t request_id;

    static read_message_t parse(stream_t *stream);

private:
    friend class local_target_t;
    friend class remote_target_t;
    read_message_t(std::vector<char> &&_buffer,
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
