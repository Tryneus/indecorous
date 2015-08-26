#include "rpc/hub.hpp"

#include <cassert>

#include "rpc/handler.hpp"
#include "rpc/target.hpp"

#include "coro/thread.hpp"

namespace indecorous {

// This constructor copies the statically-initialized set of rpcs
message_hub_t::message_hub_t() :
    m_self_target(),
    m_rpcs(register_callback(nullptr)) { }

message_hub_t::~message_hub_t() { }

future_t<read_message_t> message_hub_t::get_response(request_id_t id) {
    // TODO: need to clean up replies when losing connection to remote targets
    auto it = m_replies.find(id);
    if (it == m_replies.end()) {
        it = m_replies.emplace(id, promise_t<read_message_t>()).first;
    }
    return it->second.get_future();
}

void handle_wrapper(message_hub_t *hub, rpc_callback_t *rpc, read_message_t msg) {
    target_id_t source_id = msg.source_id;
    write_message_t reply = rpc->handle(std::move(msg));

    target_t *source = hub->target(source_id);
    if (source != nullptr) {
        source->send_reply(std::move(reply));
    }
}
void handle_noreply_wrapper(rpc_callback_t *rpc, read_message_t msg) {
    rpc->handle_noreply(std::move(msg));
}

bool message_hub_t::spawn(read_message_t msg) {
    assert(msg.buffer.has());
    if (msg.rpc_id == rpc_id_t::reply()) {
        auto reply_it = m_replies.find(msg.request_id);
        if (reply_it != m_replies.end()) {
            reply_it->second.fulfill(std::move(msg));
        } else {
            debugf("Orphan reply encountered, no promise found");
        }
    } else {
        if (msg.source_id.is_local()) {
            thread_t::self()->dispatcher()->note_accepted_task();
        }

        auto cb_it = m_rpcs.find(msg.rpc_id);
        if (msg.request_id == request_id_t::noreply()) {
            coro_t::spawn(&handle_noreply_wrapper, cb_it->second, std::move(msg));
        } else if (cb_it != m_rpcs.end()) {
            coro_t::spawn(&handle_wrapper, this, cb_it->second, std::move(msg));
        } else {
            debugf("No registered RPC for rpc_id (%lu).", msg.rpc_id.value());
        }
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
