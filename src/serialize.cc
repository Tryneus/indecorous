#include "serialize.hpp"

#include <endian.h>

#include "message.hpp"

#define IMPL_SERIALIZABLE_BYTE(Type) \
    size_t serializer_t<Type>::size(const Type &) { return 1; } \
    int serializer_t<Type>::write(write_message_t *msg, const Type &item) { \
        msg->buffer.push_back(static_cast<char>(item)); \
        return 0; \
    } \
    Type serializer_t<Type>::read(read_message_t *msg) { \
        return static_cast<Type>(msg->pop()); \
    }

#define IMPL_SERIALIZABLE_INTEGRAL(Type, Bits) \
    union Type##_wrapper_t { Type t; char c[sizeof(Type)]; }; \
    size_t serializer_t<Type>::size(const Type &) { \
        return sizeof(Type); \
    } \
    int serializer_t<Type>::write(write_message_t *msg, const Type &item) { \
        Type##_wrapper_t u; \
        u.t = htobe##Bits(item); \
        for (size_t i = 0; i < sizeof(Type); ++i) { \
            msg->buffer.push_back(u.c[i]); \
        } \
        return 0; \
    } \
    Type serializer_t<Type>::read(read_message_t *msg) { \
        Type##_wrapper_t u; \
        for (size_t i = 0; i < sizeof(Type); ++i) { \
            u.c[i] = msg->pop(); \
        } \
        return be##Bits##toh(u.t); \
    }

IMPL_SERIALIZABLE_BYTE(bool);

IMPL_SERIALIZABLE_INTEGRAL(char16_t, 16);
IMPL_SERIALIZABLE_INTEGRAL(char32_t, 32);

IMPL_SERIALIZABLE_BYTE(int8_t);
IMPL_SERIALIZABLE_INTEGRAL(int16_t, 16);
IMPL_SERIALIZABLE_INTEGRAL(int32_t, 32);
IMPL_SERIALIZABLE_INTEGRAL(int64_t, 64);

IMPL_SERIALIZABLE_BYTE(uint8_t);
IMPL_SERIALIZABLE_INTEGRAL(uint16_t, 16);
IMPL_SERIALIZABLE_INTEGRAL(uint32_t, 32);
IMPL_SERIALIZABLE_INTEGRAL(uint64_t, 64);

IMPL_SERIALIZABLE_INTEGRAL(float, 32);
IMPL_SERIALIZABLE_INTEGRAL(double, 64);

class unused_sizer_t {
    static_assert(sizeof(bool) == 1, "bool type has unexpected length");

    static_assert(sizeof(char16_t) == 2, "char16_t type has unexpected length");
    static_assert(sizeof(char32_t) == 4, "char32_t type has unexpected length");

    static_assert(sizeof(int8_t) == 1, "int8_t type has unexpected length");
    static_assert(sizeof(int16_t) == 2, "int16_t type has unexpected length");
    static_assert(sizeof(int32_t) == 4, "int32_t type has unexpected length");
    static_assert(sizeof(int64_t) == 8, "int64_t type has unexpected length");

    static_assert(sizeof(uint8_t) == 1, "uint8_t type has unexpected length");
    static_assert(sizeof(uint16_t) == 2, "uint16_t type has unexpected length");
    static_assert(sizeof(uint32_t) == 4, "uint32_t type has unexpected length");
    static_assert(sizeof(uint64_t) == 8, "uint64_t type has unexpected length");

    static_assert(sizeof(float) == 4, "float type has unexpected length");
    static_assert(sizeof(double) == 8, "double type has unexpected length");
};


