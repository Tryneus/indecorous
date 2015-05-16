#ifndef SERIALIZE_MACROS_HPP_
#define SERIALIZE_MACROS_HPP_

#include <type_traits>

#include "serialize.hpp"

#define SERIALIZABLE_INTEGRAL(Type) \
    template <> struct serializer_t<Type> { \
        static size_t size(const Type &); \
        static int write(write_message_t *, const Type &); \
        static Type read(read_message_t *); \
    };

#define SERIALIZABLE_ENUM(Type) \
    template <> struct serializer_t<Type> { \
        typedef std::underlying_type<Type>::type Value; \
        static size_t size(const Type &t) { \
            return serializer_t<Value>::size(static_cast<Value>(t)); } \
        static int write(write_message_t *msg, const Type &t) { \
            return serializer_t<Value>::write(msg, static_cast<Value>(t)); } \
        static Type read(read_message_t *msg) { \
            return static_cast<Type>(serializer_t<Value>::read(msg)); } \
    };

#define MAKE_DECLTYPE_PARAM(x) decltype(x) arg_##x
#define MAKE_MOVE_CONSTRUCT(x) x(std::move(arg_##x))
#define MAKE_DECLTYPE(x) decltype(x)

// TODO: consider wrapping these instide a static class to reduce verbosity and code generation
#define FRIEND_SERIALIZERS() \
    template <typename T> friend struct serializer_t; \
    template <typename T, size_t... N, typename... Args> \
    friend T full_deserialize_internal(std::integer_sequence<size_t, N...>, std::tuple<Args...>); \
    template <typename T, typename... Args> \
    friend T full_deserialize(read_message_t *) \

#define CONCAT(a,b) a##b

// Put a DECLARE call in the header file and an IMPL call in the implementation
// file so we don't cause as much preprocessor load in the header.
#define DECLARE_SERIALIZABLE(Type, ...) \
    DECLARE_SERIALIZABLE_INTERNAL(ZERO_OR_N(__VA_ARGS__), Type, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define DECLARE_SERIALIZABLE_INTERNAL(X, ...) \
    CONCAT(DECLARE_SERIALIZABLE_,X)(__VA_ARGS__)

#define MAKE_SERIALIZABLE(Type, ...) \
    MAKE_SERIALIZABLE_INTERNAL(ZERO_OR_N(__VA_ARGS__), Type, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define MAKE_SERIALIZABLE_INTERNAL(X, ...) \
    CONCAT(MAKE_SERIALIZABLE_,X)(__VA_ARGS__)

#define IMPL_SERIALIZABLE(Type, ...) \
    IMPL_SERIALIZABLE_INTERNAL(ZERO_OR_N(__VA_ARGS__), Type, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define IMPL_SERIALIZABLE_INTERNAL(X, ...) \
    CONCAT(IMPL_SERIALIZABLE_,X)(__VA_ARGS__)

// Function declarations and implementations for each member function needed by
// serialization - Zero-arity serializable types are not given a constructor function.
#define DECLARE_SERIALIZED_SIZE_FN(Prefix) \
    size_t Prefix serialized_size() const
#define IMPL_SERIALIZED_SIZE_BODY(...) \
    { return full_serialized_size(__VA_ARGS__); }

#define DECLARE_SERIALIZE_FN(Prefix, Param) \
    size_t Prefix serialize(write_message_t *Param) const
#define IMPL_SERIALIZE_BODY(Param, ...) \
    { return full_serialize(Param, __VA_ARGS__); }

#define DECLARE_DESERIALIZE_FN(Type, Prefix, Param) \
    Type Prefix deserialize(read_message_t *Param)
#define IMPL_DESERIALIZE_BODY(Type, Param, N, ...) \
    { return full_deserialize<Type, CALL_MACRO(N, MAKE_DECLTYPE, __VA_ARGS__)>(Param); }

#define DECLARE_CONSTRUCTOR_FN(Type, Prefix, N, ...) \
    Prefix Type(CALL_MACRO(N, MAKE_DECLTYPE_PARAM, __VA_ARGS__))
#define IMPL_CONSTRUCTOR_BODY(N, ...) \
    : CALL_MACRO(N, MAKE_MOVE_CONSTRUCT, __VA_ARGS__) { }

#define DECLARE_SERIALIZABLE_0(Type, ...) \
    DECLARE_SERIALIZED_SIZE_FN(); \
    DECLARE_SERIALIZE_FN(,); \
    static DECLARE_DESERIALIZE_FN(Type,,); \
    FRIEND_SERIALIZERS()

#define DECLARE_SERIALIZABLE_N(Type, ...) \
    DECLARE_SERIALIZED_SIZE_FN(); \
    DECLARE_SERIALIZE_FN(,msg); \
    static DECLARE_DESERIALIZE_FN(Type,,msg); \
    Type(CALL_MACRO(N, MAKE_DECLTYPE_PARAM, __VA_ARGS__)); \
    FRIEND_SERIALIZERS()

#define MAKE_SERIALIZABLE_0(Type, ...) \
    DECLARE_SERIALIZED_SIZE_FN() { return 0; } \
    DECLARE_SERIALIZE_FN(,) { return 0; } \
    static DECLARE_DESERIALIZE_FN(Type,,) { return Type(); } \
    FRIEND_SERIALIZERS()

#define MAKE_SERIALIZABLE_N(Type, N, ...) \
    DECLARE_SERIALIZED_SIZE_FN() IMPL_SERIALIZED_SIZE_BODY(__VA_ARGS__) \
    DECLARE_SERIALIZE_FN(,msg) IMPL_SERIALIZE_BODY(msg, __VA_ARGS__) \
    static DECLARE_DESERIALIZE_FN(Type,,msg) IMPL_DESERIALIZE_BODY(Type, msg, N, __VA_ARGS__) \
    DECLARE_CONSTRUCTOR_FN(Type,,N,__VA_ARGS__) IMPL_CONSTRUCTOR_BODY(N, __VA_ARGS__) \
    FRIEND_SERIALIZERS()

#define IMPL_SERIALIZABLE_0(Type, ...) \
    DECLARE_SERIALIZED_SIZE_FN(Type::) { return 0; } \
    DECLARE_SERIALIZE_FN(Type::,) { return 0; } \
    DECLARE_DESERIALIZE_FN(Type, Type::,) { return Type(); }

#define IMPL_SERIALIZABLE_N(Type, ...) \
    DECLARE_SERIALIZED_SIZE_FN(Type::) IMPL_SERIALIZED_SIZE_BODY(__VA_ARGS__) \
    DECLARE_SERIALIZE_FN(Type::,msg) IMPL_SERIALIZE_BODY(msg, __VA_ARGS__) \
    static DECLARE_DESERIALIZE_FN(Type,Type::,msg) IMPL_DESERIALIZE_BODY(Type, msg, N, __VA_ARGS__) \
    DECLARE_CONSTRUCTOR_FN(Type,Type::,N,__VA_ARGS__) IMPL_CONSTRUCTOR_BODY(N, __VA_ARGS__)

// These macros allow SERIALIZABLE to be called with up to 32 member variables,
// this can be extended by continuing the sequences in these macros.
#define NUM_ARGS(...) NUM_ARGS_(__VA_ARGS__,32,31,30,29,28,27,26,25,24,23,22,21,20 \
                                19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define NUM_ARGS_( _1, _2, _3, _4, _5, _6, _7, _8, \
                   _9,_10,_11,_12,_13,_14,_15,_16, \
                  _17,_18,_19,_20,_21,_22,_23,_24, \
                  _25,_26,_27,_28,_29,_30,_31,X,...) X

// These macros choose between SERIALIZABLE_0 and SERIALIZABLE_N
#define ZERO_OR_N(...) ZERO_OR_N_(__VA_ARGS__,N,N,N,N,N,N,N,N,N,N,N,N \
                                  N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,N,0)
#define ZERO_OR_N_( _1, _2, _3, _4, _5, _6, _7, _8, \
                    _9,_10,_11,_12,_13,_14,_15,_16, \
                   _17,_18,_19,_20,_21,_22,_23,_24, \
                   _25,_26,_27,_28,_29,_30,_31,X,...) X

#define CALL_MACRO(N, name, ...) CALL_MACRO_##N(name, __VA_ARGS__)
#define CALL_MACRO_32(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_31(name, __VA_ARGS__)
#define CALL_MACRO_31(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_30(name, __VA_ARGS__)
#define CALL_MACRO_30(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_29(name, __VA_ARGS__)
#define CALL_MACRO_29(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_28(name, __VA_ARGS__)
#define CALL_MACRO_28(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_27(name, __VA_ARGS__)
#define CALL_MACRO_27(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_26(name, __VA_ARGS__)
#define CALL_MACRO_26(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_25(name, __VA_ARGS__)
#define CALL_MACRO_25(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_24(name, __VA_ARGS__)
#define CALL_MACRO_24(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_23(name, __VA_ARGS__)
#define CALL_MACRO_23(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_22(name, __VA_ARGS__)
#define CALL_MACRO_22(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_21(name, __VA_ARGS__)
#define CALL_MACRO_21(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_20(name, __VA_ARGS__)
#define CALL_MACRO_20(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_19(name, __VA_ARGS__)
#define CALL_MACRO_19(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_18(name, __VA_ARGS__)
#define CALL_MACRO_18(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_17(name, __VA_ARGS__)
#define CALL_MACRO_17(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_16(name, __VA_ARGS__)
#define CALL_MACRO_16(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_15(name, __VA_ARGS__)
#define CALL_MACRO_15(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_14(name, __VA_ARGS__)
#define CALL_MACRO_14(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_13(name, __VA_ARGS__)
#define CALL_MACRO_13(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_12(name, __VA_ARGS__)
#define CALL_MACRO_12(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_11(name, __VA_ARGS__)
#define CALL_MACRO_11(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_10(name, __VA_ARGS__)
#define CALL_MACRO_10(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_9(name, __VA_ARGS__)
#define CALL_MACRO_9(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_8(name, __VA_ARGS__)
#define CALL_MACRO_8(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_7(name, __VA_ARGS__)
#define CALL_MACRO_7(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_6(name, __VA_ARGS__)
#define CALL_MACRO_6(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_5(name, __VA_ARGS__)
#define CALL_MACRO_5(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_4(name, __VA_ARGS__)
#define CALL_MACRO_4(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_3(name, __VA_ARGS__)
#define CALL_MACRO_3(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_2(name, __VA_ARGS__)
#define CALL_MACRO_2(name, x, ...) CALL_MACRO_1(name, x), CALL_MACRO_1(name, __VA_ARGS__)
#define CALL_MACRO_1(name, x) name(x)
#define CALL_MACRO_0(name)

#endif // SERIALIZE_MACROS_HPP_
