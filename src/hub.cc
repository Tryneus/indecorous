#include "hub.hpp"

#include <cassert>

#include "handler.hpp"
#include "target.hpp"

template <typename T>
message_hub_t::membership_t<T>::membership_t(message_hub_t *_hub, T *_member) :
        hub(_hub), member(_member) {
    hub->add(member);
}

template <typename T>
message_hub_t::membership_t<T>::~membership_t() {
    hub->remove(member);
}

// Instantiations of membership for the supported types
template class message_hub_t::membership_t<handler_callback_t>;
template class message_hub_t::membership_t<target_t>;

message_hub_t::message_hub_t() { }
message_hub_t::~message_hub_t() { }

target_t *message_hub_t::get_target(target_id_t id) const {
    auto it = targets.find(id);
    return (it == targets.end()) ? nullptr : it->second;
}

void message_hub_t::add(handler_callback_t *callback) {
    auto res = callbacks.insert(std::make_pair(callback->id(), callback));
    assert(res.second);
}
void message_hub_t::remove(handler_callback_t *callback) {
    size_t res = callbacks.erase(callback->id());
    assert(res == 1);
}

void message_hub_t::add(target_t *target) {
    auto res = targets.insert(std::make_pair(target->id(), target));
    assert(res.second);
}

void message_hub_t::remove(target_t *target) {
    size_t res = targets.erase(target->id());
    assert(res == 1);
}
