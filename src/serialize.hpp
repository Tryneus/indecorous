#ifndef SERIALIZE_HPP_
#define SERIALIZE_HPP_

#include <cstddef>
#include <cstdint>
#include <utility>
#include "serialize_macros.hpp"

class read_message_t;
class write_message_t;

template <typename T>
struct sizer_t {
    static size_t run(const T &item) {
        return item.serialized_size();
    }
};

template <typename T>
struct serializer_t {
    static int run(write_message_t *msg, const T &item) {
        return item.serialize(msg);
    }
};

template <typename T>
struct deserializer_t {
    static T run(read_message_t *msg) {
        return T::deserialize(msg);
    }
};

// Specializations for integral and unchangable types
template <> struct sizer_t<bool> {
    static size_t run(const bool &item);
};
template <> struct serializer_t<bool> {
    static int run(write_message_t *msg, const bool &item);
};
template <> struct deserializer_t<bool> {
    static bool run(read_message_t *msg);
};

template <> struct sizer_t<uint64_t> {
    static size_t run(const uint64_t &item);
};
template <> struct serializer_t<uint64_t> {
    static int run(write_message_t *msg, const uint64_t &item);
};
template <> struct deserializer_t<uint64_t> {
    static uint64_t run(read_message_t *msg);
};

// template <> size_t serialized_size(const size_t &item);
// template <> int serialize(write_message_t *msg, size_t &&item);
// template <> size_t deserialize(read_message_t *msg);

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

template <typename... Args>
size_t full_serialized_size(const Args &...args) {
    class accumulator_t {
    public:
        accumulator_t() : value(0) { }
        int add(size_t delta) { value += delta; return 0; }
        size_t value;
    } acc;

    __attribute__((unused)) auto dummy =
        { acc.add(sizer_t<Args>::run(args))... };
    return acc.value;
}

template <typename... Args>
int full_serialize(write_message_t *msg, const Args &...args) {
    __attribute__((unused)) auto dummy =
        { serializer_t<Args>::run(msg, args)... };
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
                                        std::tuple<Args...>{deserializer_t<Args>::run(msg)...});
}


#endif // SERIALIZE_HPP_
