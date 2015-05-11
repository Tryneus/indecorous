#ifndef ID_HPP_
#define ID_HPP_

#include <cstdint>

class target_id_t {
public:
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
    uint64_t value() const { return value_; }
    static handler_id_t assign() {
        static uint64_t next_value(0);
        return handler_id_t(next_value++);
    }
private:
    friend class read_message_t;
    handler_id_t(uint64_t _value) : value_(_value) { }
    uint64_t value_;
};

class request_id_t {
public:
    request_id_t(uint64_t _value) : value_(_value) { }
    uint64_t value() const { return value_; }
    static request_id_t noreply() { return request_id_t(-1); }
private:
    uint64_t value_;
};

#endif // ID_HPP_
