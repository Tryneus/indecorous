#include "rpc/id.hpp"

#include <limits>

namespace indecorous {

id_generator_t<target_id_t> target_id_t::generator;

target_id_t::target_id_t(uint64_t _value) :
    value_(_value) { }

target_id_t target_id_t::assign() {
    return generator.next();
}

bool target_id_t::is_local() const {
    return true; // TODO: local/remote target id generation
}

uint64_t target_id_t::value() const {
    return value_;
}

bool target_id_t::operator ==(const target_id_t &other) const {
    return value_ == other.value_;
}

rpc_id_t::rpc_id_t(uint64_t _value) :
    value_(_value) { }

uint64_t rpc_id_t::value() const {
    return value_;
}

bool rpc_id_t::operator ==(const rpc_id_t &other) const {
    return value_ == other.value_;
}

rpc_id_t rpc_id_t::reply() {
    return rpc_id_t(std::numeric_limits<uint64_t>::max());
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
    return request_id_t(std::numeric_limits<uint64_t>::max());
}

task_id_t::task_id_t(uint64_t _value) :
    value_(_value) { }

uint64_t task_id_t::value() const {
    return value_;
}

bool task_id_t::operator ==(const task_id_t &other) const {
    return value_ == other.value_;
}

} // namespace indecorous

