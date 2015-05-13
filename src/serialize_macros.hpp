
#define SERIALIZABLE_COMMON(Type, ...) \
    template <typename T> friend class sizer_t; \
    template <typename T> friend class serializer_t; \
    template <typename T> friend class deserializer_t; \
    size_t serialized_size() const { \
        return full_serialized_size(__VA_ARGS__); \
    } \

// This is special-cased
// TODO: no default constructor here as it may conflict with an existing one
#define SERIALIZABLE_0(Type) \
    template <typename T> friend class sizer_t; \
    template <typename T> friend class serializer_t; \
    template <typename T> friend class deserializer_t; \
    size_t serialized_size() const { return 0; } \
    int serialize(write_message_t *msg) && { return 0; } \
    static Type deserialize(read_message_t *msg) { return Type(); }


#define SERIALIZABLE_1(Type, a) \
    SERIALIZABLE_COMMON(Type, a) \
    int serialize(write_message_t *msg) && { \
        return full_serialize(msg, std::move(a)); \
    } \
    Type(decltype(a) &&_a) : \
        a(std::move(_a)) { } \
    static Type deserialize(read_message_t *msg) { \
        return full_deserialize<Type, decltype(a)>(msg); \
    }


#define SERIALIZABLE_2(Type, a, b) \
    SERIALIZABLE_COMMON(Type, a, b) \
    int serialize(write_message_t *msg) && { \
        return full_serialize(msg, std::move(a), std::move(b)); \
    } \
    Type(decltype(a) &&_a, decltype(b) &&_b) : \
        a(std::move(_a)), b(std::move(_b)) { } \
    static Type deserialize(read_message_t *msg) { \
        return full_deserialize<Type, decltype(a), decltype(b)>(msg); \
    }
