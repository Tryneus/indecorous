#include "rpc/hub.hpp"

#include <cassert>

#include "rpc/handler.hpp"
#include "rpc/target.hpp"

#include "coro/thread.hpp"

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
    assert(msg.buffer.has());

    auto cb_it = callbacks.find(msg.handler_id);
    if (msg.request_id == request_id_t::noreply()) {
        debugf("handling noreply rpc\n");
        coro_t::spawn(&handler_callback_t::handle_noreply, cb_it->second, std::move(msg));
    } else if (cb_it != callbacks.end()) {
        debugf("handling reply rpc\n");
        coro_t::spawn(&handler_callback_t::handle, cb_it->second, this, std::move(msg));
    } else {
        printf("No extant handler for handler_id (%lu).\n", msg.handler_id.value());
    }
    return true;
}

target_t *message_hub_t::target(target_id_t id) const {
    auto it = targets.find(id);
    return (it == targets.end()) ? nullptr : it->second;
}

void message_hub_t::send_reply(target_id_t target_id, write_message_t &&msg) {
    target_t *t = target(target_id);
    if (t != nullptr) {
        t->stream()->write(std::move(msg));
    } else {
        printf("Cannot find target (%lu) to send reply to.\n", target_id.value());
    }
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
