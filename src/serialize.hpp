#ifndef SERIALIZE_HPP_
#define SERIALIZE_HPP_

#include <cstddef>
#include <cstdint>
#include <utility>

#include "serialize_macros.hpp"

class read_message_t;
class write_message_t;

template <typename T>
struct serializer_t {
    static size_t size(const T &item) {
        return item.serialized_size();
    }
    static int write(write_message_t *msg, const T &item) {
        return item.serialize(msg);
    }
    static T read(read_message_t *msg) {
        return T::deserialize(msg);
    }
};

// Specializations for integral and unchangable types

// TODO: declare pointers unserializable (or recurse into them and use temporary storage on the other side)
// allowing pointers will mean we need to avoid loops in objects (we may have to avoid those anyway what with
// references...).

// TODO: would be nice to error if someone tries to send a size_t
// probably impossible - just refuse to connect to different bit-size archs
SERIALIZABLE_INTEGRAL(bool);
SERIALIZABLE_INTEGRAL(char16_t);
SERIALIZABLE_INTEGRAL(char32_t);
SERIALIZABLE_INTEGRAL(int8_t);
SERIALIZABLE_INTEGRAL(int16_t);
SERIALIZABLE_INTEGRAL(int32_t);
SERIALIZABLE_INTEGRAL(int64_t);
SERIALIZABLE_INTEGRAL(uint8_t);
SERIALIZABLE_INTEGRAL(uint16_t);
SERIALIZABLE_INTEGRAL(uint32_t);
SERIALIZABLE_INTEGRAL(uint64_t);
SERIALIZABLE_INTEGRAL(float);
SERIALIZABLE_INTEGRAL(double);

template <typename... Args>
size_t full_serialized_size(const Args &...args) {
    class accumulator_t {
    public:
        accumulator_t() : value(0) { }
        int add(size_t delta) { value += delta; return 0; }
        size_t value;
    } acc;

    __attribute__((unused)) auto dummy =
        { acc.add(serializer_t<Args>::size(args))... };
    return acc.value;
}

template <typename... Args>
int full_serialize(write_message_t *msg, const Args &...args) {
    __attribute__((unused)) auto dummy =
        { serializer_t<Args>::write(msg, args)... };
    return 0;
}

template <typename T, size_t... N, typename... Args>
T full_deserialize_internal(std::integer_sequence<size_t, N...>,
                            std::tuple<Args...> args) {
    return T(std::move(std::get<N>(args))...);
}

template <typename T, typename... Args>
T full_deserialize(read_message_t *msg) {
    return full_deserialize_internal<T>(std::index_sequence_for<Args...>{},
                                        std::tuple<Args...>{serializer_t<Args>::read(msg)...});
}


#endif // SERIALIZE_HPP_
