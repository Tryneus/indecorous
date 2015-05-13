#ifndef SERIALIZE_HPP_
#define SERIALIZE_HPP_

#include <cstddef>
#include <cstdint>
#include <utility>

class read_message_t;
class write_message_t;

// These functions must be implemented on any serializable class
#define DECLARE_SERIALIZABLE(Type) \
    size_t serialized_size() const; \
    int serialize(write_message_t *msg); \
    static Type deserialize(read_message_t *msg)

template <typename T>
struct sizer_t {
    static size_t run(const T &item) {
        return item.serialized_size();
    }
};

template <typename T>
struct serializer_t {
    static int run(write_message_t *msg, T &&item) {
        return std::move(item).serialize(msg);
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
    static int run(write_message_t *msg, bool &&item);
};
template <> struct deserializer_t<bool> {
    static bool run(read_message_t *msg);
};

template <> struct sizer_t<uint64_t> {
    static size_t run(const uint64_t &item);
};
template <> struct serializer_t<uint64_t> {
    static int run(write_message_t *msg, uint64_t &&item);
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
int full_serialize(write_message_t *msg, Args &&...args) {
    __attribute__((unused)) auto dummy =
        { serializer_t<Args>::run(msg, std::forward<Args>(args))... };
    return 0;
}

#endif // SERIALIZE_HPP_
