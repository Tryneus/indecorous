#ifndef SERIALIZE_MACROS_HPP_
#define SERIALIZE_MACROS_HPP_

#define FRIEND_SERIALIZERS() \
    template <typename T> friend struct sizer_t; \
    template <typename T> friend struct serializer_t; \
    template <typename T> friend struct deserializer_t; \
    template <typename T, size_t... N, typename... Args> \
    friend T full_deserialize_internal(std::integer_sequence<size_t, N...>, std::tuple<Args...> &&)

#define SERIALIZED_SIZE(...) \
    size_t serialized_size() const { \
        return full_serialized_size(__VA_ARGS__); \
    } \

#define MAKE_MOVE(x) std::move(x)
#define MAKE_DECLTYPE_PARAM(x) decltype(x) arg_##x
#define MAKE_CONSTRUCT(x) x(std::move(arg_##x))
#define MAKE_DECLTYPE(x) decltype(x)

// This is special-cased
// TODO: no default constructor here as it may conflict with an existing one
#define SERIALIZABLE_0(Type) \
    size_t serialized_size() const { return 0; } \
    int serialize(write_message_t *msg) && { return 0; } \
    static Type deserialize(read_message_t *msg) { return Type(); } \
    FRIEND_SERIALIZERS()

#define SERIALIZABLE(Type, ...) SERIALIZABLE_N(Type, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

#define SERIALIZABLE_N(Type, N, ...) \
    int serialize(write_message_t *msg) && { \
        return full_serialize(msg, CALL_MACRO(N, MAKE_MOVE, __VA_ARGS__)); \
    } \
    Type(CALL_MACRO(N, MAKE_DECLTYPE_PARAM, __VA_ARGS__)) : \
        CALL_MACRO(N, MAKE_CONSTRUCT, __VA_ARGS__) { } \
    static Type deserialize(read_message_t *msg) { \
        return full_deserialize<Type, CALL_MACRO(N, MAKE_DECLTYPE, __VA_ARGS__)>(msg); \
    } \
    SERIALIZED_SIZE(__VA_ARGS__) \
    FRIEND_SERIALIZERS()

// These macros allow SERIALIZABLE to be called with up to 32 member variables,
// this can be extended by continuing the sequences in these two macros.
#define NUM_ARGS(...) NUM_ARGS_N(__VA_ARGS__,32,31,30,29,28,27,26,25,24,23,22,21,20 \
                                 19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define NUM_ARGS_N( _1, _2, _3, _4, _5, _6, _7, _8, \
                    _9,_10,_11,_12,_13,_14,_15,_16, \
                   _17,_18,_19,_20,_21,_22,_23,_24, \
                   _25,_26,_27,_28,_29,_30,_31,N,...) N

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

#endif // SERIALIZE_MACROS_HPP_
