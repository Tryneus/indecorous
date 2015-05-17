#ifndef RPC_ID_HPP_
#define RPC_ID_HPP_

#include <cstdint>
#include <cstddef>

#include <functional>

namespace indecorous {

template <typename T>
class id_equal_t {
public:
    bool operator() (const T &lhs, const T &rhs) const {
        return lhs.value() == rhs.value();
    }
};

template <typename T>
class id_hash_t {
public:
    constexpr size_t operator () (const T &item) const {
        return std::hash<uint64_t>()(item.value());
    }
};

class target_id_t {
public:
    typedef id_hash_t<target_id_t> hash_t;
    typedef id_equal_t<target_id_t> equal_t;

    uint64_t value() const { return value_; }
    static target_id_t assign() {
        static uint64_t next_value(0);
        return target_id_t(next_value++);
    }
private:
    friend class read_message_t;
    target_id_t(uint64_t _value) : value_(_value) { }
    uint64_t value_;
};

class handler_id_t {
public:
    typedef id_hash_t<handler_id_t> hash_t;
    typedef id_equal_t<handler_id_t> equal_t;

    uint64_t value() const { return value_; }
    static handler_id_t assign() {
        static uint64_t next_value(0);
        return handler_id_t(next_value++);
    }
    static handler_id_t reply() { return handler_id_t(-1); }
private:
    friend class read_message_t;
    handler_id_t(uint64_t _value) : value_(_value) { }
    uint64_t value_;
};

class request_id_t {
public:
    typedef id_hash_t<request_id_t> hash_t;
    typedef id_equal_t<request_id_t> equal_t;

    request_id_t(uint64_t _value) : value_(_value) { }
    uint64_t value() const { return value_; }
    static request_id_t noreply() { return request_id_t(-1); }
private:
    uint64_t value_;
};

} // namespace indecorous

#endif // ID_HPP_
