#include "serialize.hpp"

#include <endian.h>

#include "debug.hpp"
#include "message.hpp"

// Specializations for integral and unchangable types
size_t sizer_t<bool>::run(const bool &) {
    return 1;
}
int serializer_t<bool>::run(write_message_t *msg, bool &&item) {
    msg->buffer.push_back(static_cast<char>(item));
    return 0;
}
bool deserializer_t<bool>::run(read_message_t *msg) {
    return static_cast<bool>(msg->pop());
}

size_t sizer_t<uint64_t>::run(const uint64_t &item) {
    return sizeof(item);
}
int serializer_t<uint64_t>::run(write_message_t *msg, uint64_t &&item) {
    uint64_t buffer = htobe64(item);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        msg->buffer.push_back(reinterpret_cast<char *>(&buffer)[i]);
    }
    return 0;
}
uint64_t deserializer_t<uint64_t>::run(read_message_t *msg) {
    uint64_t buffer;
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        reinterpret_cast<char *>(&buffer)[i] = msg->pop();
    }
    return be64toh(buffer);
}

// template <> size_t serialized_size(const size_t &item) {
//     return serialized_size(uint64_t(item));
// }
// template <> int serialize(write_message_t *msg, size_t &&item) {
//     return serialize(msg, uint64_t(item));
// }
// template <> size_t deserialize(read_message_t *msg) {
//     return deserialize<uint64_t>(msg);
// }

/*
template<> size_t deserialize(read_message_t *msg);
template<> int8_t deserialize(read_message_t *msg);
template<> uint8_t deserialize(read_message_t *msg);
template<> int16_t deserialize(read_message_t *msg);
template<> uint16_t deserialize(read_message_t *msg);
template<> int32_t deserialize(read_message_t *msg);
template<> uint32_t deserialize(read_message_t *msg);
template<> int64_t deserialize(read_message_t *msg);
template<> uint64_t deserialize(read_message_t *msg);
*/
