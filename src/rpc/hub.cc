#include "rpc/hub.hpp"

#include <cassert>

#include "rpc/handler.hpp"
#include "rpc/hub_data.hpp"
#include "rpc/target.hpp"

#include "coro/thread.hpp"

namespace indecorous {

// This constructor copies the statically-initialized set of handlers
message_hub_t::message_hub_t() :
    m_self_target(),
    m_handlers(register_handler(nullptr)) { }

message_hub_t::~message_hub_t() { }

bool message_hub_t::spawn(read_message_t msg) {
    assert(msg.buffer.has());

    auto cb_it = m_handlers.find(msg.handler_id);
    if (msg.request_id == request_id_t::noreply()) {
        coro_t::spawn(&handler_callback_t::handle_noreply, cb_it->second, std::move(msg));
    } else if (cb_it != m_handlers.end()) {
        coro_t::spawn(&handler_callback_t::handle, cb_it->second, this, std::move(msg));
    } else {
        debugf("No registered handler for handler_id (%lu).\n", msg.handler_id.value());
    }
    return true;
}

local_target_t *message_hub_t::self_target() {
    return &m_self_target;
}

target_t *message_hub_t::target(target_id_t id) {
    auto it = m_targets.find(id);
    return (it == m_targets.end()) ? nullptr : it->second;
}

void message_hub_t::send_reply(target_id_t target_id, write_message_t &&msg) {
    target_t *t = target(target_id);
    if (t != nullptr) {
        t->send_reply(std::move(msg));
    } else {
        debugf("Cannot find target (%lu) to send reply to.\n", target_id.value());
    }
}

void message_hub_t::set_local_targets(std::list<thread_t> *threads) {
    // This should be done once and only once during startup of the thread pool
    assert(m_local_targets.empty());
    m_local_targets.reserve(threads->size());
    for (auto &&t : *threads) {
        local_target_t *thread_target = t.hub()->self_target();
        m_local_targets.emplace_back(thread_target);
        m_targets.emplace(thread_target->id(), thread_target);
    }
}

} // namespace indecorous
