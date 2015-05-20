#ifndef RPC_ID_HPP_
#define RPC_ID_HPP_

#include <cstdint>
#include <cstddef>

#include <functional>

namespace indecorous {

template <typename id_t>
class id_generator_t {
public:
    id_generator_t() : next_id(0) { }
    id_t next() { return id_t(next_id++); }
private:
    uint64_t next_id;
};

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

    // This should only be called before spawning threads
    static target_id_t assign();

    uint64_t value() const;
    bool operator <(const target_id_t &other) const;
private:
    static id_generator_t<target_id_t> generator;

    friend class id_generator_t<target_id_t>;
    friend class read_message_t;
    target_id_t(uint64_t _value);
    uint64_t value_;
};

class handler_id_t {
public:
    typedef id_hash_t<handler_id_t> hash_t;
    typedef id_equal_t<handler_id_t> equal_t;

    uint64_t value() const;
    static handler_id_t reply();
private:
    friend class id_generator_t<handler_id_t>;
    friend class read_message_t;
    handler_id_t(uint64_t _value);
    uint64_t value_;
};

class request_id_t {
public:
    typedef id_hash_t<request_id_t> hash_t;
    typedef id_equal_t<request_id_t> equal_t;

    uint64_t value() const;
    static request_id_t noreply();
private:
    friend class id_generator_t<request_id_t>;
    friend class read_message_t;
    request_id_t(uint64_t _value);
    uint64_t value_;
};

} // namespace indecorous

#endif // ID_HPP_
