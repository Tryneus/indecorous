#include "rpc/id.hpp"

namespace indecorous {

id_generator_t<target_id_t> target_id_t::generator;

target_id_t::target_id_t(uint64_t _value) :
    value_(_value) { }

target_id_t target_id_t::assign() {
    return generator.next();
}

uint64_t target_id_t::value() const {
    return value_;
}

bool target_id_t::operator ==(const target_id_t &other) const {
    return value_ == other.value_;
}

handler_id_t::handler_id_t(uint64_t _value) :
    value_(_value) { }

uint64_t handler_id_t::value() const {
    return value_;
}

bool handler_id_t::operator ==(const handler_id_t &other) const {
    return value_ == other.value_;
}

handler_id_t handler_id_t::reply() {
    return handler_id_t(-1);
}

request_id_t::request_id_t(uint64_t _value) :
    value_(_value) { }

uint64_t request_id_t::value() const {
    return value_;
}

bool request_id_t::operator ==(const request_id_t &other) const {
    return value_ == other.value_;
}

request_id_t request_id_t::noreply() {
    return request_id_t(-1);
}

} // namespace indecorous

