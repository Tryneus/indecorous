#include "rpc/hub.hpp"

#include <cassert>

#include "rpc/handler.hpp"
#include "rpc/target.hpp"

namespace indecorous {

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

bool message_hub_t::spawn(read_message_t msg) {
    if (!msg.buffer.has()) return false;

    auto cb_it = callbacks.find(msg.handler_id);
    if (cb_it == callbacks.end()) {
        printf("No extant handler for handler_id (%lu).\n", msg.handler_id.value());
    } else if (msg.request_id == request_id_t::noreply()) {
        coro_t::spawn(&handler_callback_t::handle_noreply, cb_it->second, &msg);
    } else {
        coro_t::spawn(&handler_callback_t::handle, cb_it->second, &msg);
    }
    return true;
}

target_t *message_hub_t::target(target_id_t id) const {
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

void message_hub_t::add(target_t *_target) {
    auto res = targets.insert(std::make_pair(_target->id(), _target));
    assert(res.second);
}

void message_hub_t::remove(target_t *_target) {
    size_t res = targets.erase(_target->id());
    assert(res == 1);
}

} // namespace indecorous
