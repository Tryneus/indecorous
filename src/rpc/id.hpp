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

class target_id_t {
public:
    // This should only be called before spawning threads
    static target_id_t assign();

    uint64_t value() const;
    bool operator ==(const target_id_t &other) const;
private:
    static id_generator_t<target_id_t> generator;

    friend class id_generator_t<target_id_t>;
    friend class read_message_t;
    explicit target_id_t(uint64_t _value);
    uint64_t value_;
};

class handler_id_t {
public:
    uint64_t value() const;
    static handler_id_t reply();
    bool operator ==(const handler_id_t &other) const;
private:
    friend class id_generator_t<handler_id_t>;
    friend class read_message_t;
    explicit handler_id_t(uint64_t _value);
    uint64_t value_;
};

class request_id_t {
public:
    uint64_t value() const;
    static request_id_t noreply();
    bool operator ==(const request_id_t &other) const;
private:
    friend class id_generator_t<request_id_t>;
    friend class read_message_t;
    explicit request_id_t(uint64_t _value);
    uint64_t value_;
};

} // namespace indecorous

namespace std {

template <> struct hash<indecorous::target_id_t> {
    size_t operator () (const indecorous::target_id_t &id) const {
        return std::hash<uint64_t>()(id.value());
    }
};
template <> struct hash<indecorous::handler_id_t> {
    size_t operator () (const indecorous::handler_id_t &id) const {
        return std::hash<uint64_t>()(id.value());
    }
};
template <> struct hash<indecorous::request_id_t> {
    size_t operator () (const indecorous::request_id_t &id) const {
        return std::hash<uint64_t>()(id.value());
    }
};

} // namespace std

#endif // ID_HPP_
