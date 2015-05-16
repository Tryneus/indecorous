#include "serialize.hpp"

#include <endian.h>

#include "message.hpp"

/*
SERIALIZABLE_INTEGRAL(char16_t);
SERIALIZABLE_INTEGRAL(char32_t);
SERIALIZABLE_INTEGRAL(int8_t);
SERIALIZABLE_INTEGRAL(int16_t);
SERIALIZABLE_INTEGRAL(int32_t);
SERIALIZABLE_INTEGRAL(int64_t);
SERIALIZABLE_INTEGRAL(uint8_t);
SERIALIZABLE_INTEGRAL(uint16_t);
SERIALIZABLE_INTEGRAL(uint32_t);
SERIALIZABLE_INTEGRAL(float);
SERIALIZABLE_INTEGRAL(double);
SERIALIZABLE_INTEGRAL(long double);
*/

// Specializations for integral and unchangable types
size_t serializer_t<bool>::size(const bool &) {
    return 1;
}
int serializer_t<bool>::write(write_message_t *msg, const bool &item) {
    msg->buffer.push_back(static_cast<char>(item));
    return 0;
}
bool serializer_t<bool>::read(read_message_t *msg) {
    return static_cast<bool>(msg->pop());
}

size_t serializer_t<uint64_t>::size(const uint64_t &item) {
    return sizeof(item);
}
int serializer_t<uint64_t>::write(write_message_t *msg, const uint64_t &item) {
    uint64_t buffer = htobe64(item);
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        msg->buffer.push_back(reinterpret_cast<char *>(&buffer)[i]);
    }
    return 0;
}
uint64_t serializer_t<uint64_t>::read(read_message_t *msg) {
    uint64_t buffer;
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        reinterpret_cast<char *>(&buffer)[i] = msg->pop();
    }
    return be64toh(buffer);
}
