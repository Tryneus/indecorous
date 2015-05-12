#include "hub.hpp"

#include <cassert>

#include "handler.hpp"
#include "target.hpp"

message_hub_t::message_hub_t() { }
message_hub_t::~message_hub_t() { }

target_t *message_hub_t::get_target(target_id_t id) const {
    auto it = targets.find(id);
    return (it == targets.end()) ? nullptr : it->second;
}

message_callback_t *message_hub_t::get_handler(handler_id_t id) const {
    auto it = callbacks.find(id);
    return (it == callbacks.end()) ? nullptr : it->second;
}

void message_hub_t::add_handler(message_callback_t *callback) {
    auto res = callbacks.insert(std::make_pair(callback->id(), callback));
    assert(res.second);
}
void message_hub_t::remove_handler(message_callback_t *callback) {
    size_t res = callbacks.erase(callback->id());
    assert(res == 1);
}

void message_hub_t::add_target(target_t *target) {
    auto res = targets.insert(std::make_pair(target->id(), target));
    assert(res.second);
}

void message_hub_t::remove_target(target_t *target) {
    size_t res = targets.erase(target->id());
    assert(res == 1);
}
