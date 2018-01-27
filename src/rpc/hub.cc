#include "rpc/hub.hpp"

#include <cassert>

#include "rpc/handler.hpp"
#include "rpc/target.hpp"

#include "coro/thread.hpp"

namespace indecorous {

// This constructor copies the statically-initialized set of rpcs
message_hub_t::message_hub_t(target_t *self_target, target_t *_io_target) :
    m_self_target_id(self_target->id()),
    m_io_target(_io_target),
    m_local_targets(),
    m_targets(),
    m_rpcs(register_callback(nullptr)),
    m_request_gen(),
    m_running(),
    m_replies() { }

message_hub_t::~message_hub_t() { }

void message_hub_t::handle_wrapper(rpc_callback_t *rpc, read_message_t msg) {
    target_id_t source_id = msg.source_id;
    request_id_t request_id = msg.request_id;
    write_message_t reply = rpc->handle(std::move(msg));

    target_t *source = target(source_id);
    if (source != nullptr) {
        source->send_reply(std::move(reply));
    } else {
        debugf("Could not find target (%" PRIu64 ") for reply, might be disconnected", source_id.value());
    }

    auto res = m_running.erase(std::make_pair(source_id, request_id));
    GUARANTEE(res == 1);
}

void message_hub_t::handle_noreply_wrapper(rpc_callback_t *rpc, read_message_t msg) {
    target_id_t source_id = msg.source_id;
    request_id_t request_id = msg.request_id;

    rpc->handle_noreply(std::move(msg));

    auto res = m_running.erase(std::make_pair(source_id, request_id));
    GUARANTEE(res == 1);
}

target_t::request_params_t message_hub_t::new_request() {
    request_id_t request_id = m_request_gen.next();
    auto res = m_replies.emplace(request_id, promise_t<read_message_t>());
    assert(res.second);
    return { m_self_target_id, request_id, res.first->second.get_future() };
}

void message_hub_t::spawn_task(read_message_t msg) {
    assert(msg.buffer.has());
    if (msg.rpc_id == rpc_id_t::reply()) {
        auto reply_it = m_replies.find(msg.request_id);
        if (reply_it != m_replies.end()) {
            reply_it->second.fulfill(std::move(msg));
        } else {
            debugf("Orphan reply encountered, no promise found.");
        }
    } else {
        if (msg.source_id.is_local()) {
            thread_t::self()->dispatcher()->note_accepted_task();
        }

        auto cb_it = m_rpcs.find(msg.rpc_id);
        if (msg.request_id == request_id_t::noreply()) {
            auto result = coro_t::spawn(&message_hub_t::handle_noreply_wrapper, this, cb_it->second, std::move(msg));
            m_running.insert(
                std::make_pair(
                    std::make_pair(msg.source_id, msg.request_id),
                    result.detach()
                )
            );
        } else if (cb_it != m_rpcs.end()) {
            auto result = coro_t::spawn(&message_hub_t::handle_wrapper, this, cb_it->second, std::move(msg));
            m_running.insert(
                std::make_pair(
                    std::make_pair(msg.source_id, msg.request_id),
                    result.detach()
                )
            );
        } else {
            debugf("No registered RPC for rpc_id (%lu).", msg.rpc_id.value());
        }
    }
}

target_t *message_hub_t::target(target_id_t id) {
    auto it = m_targets.find(id);
    return (it == m_targets.end()) ? nullptr : it->second;
}

target_t *message_hub_t::io_target() {
    return m_io_target;
}

const std::vector<target_t *> &message_hub_t::local_targets() {
    return m_local_targets;
}

void message_hub_t::add_local_target(target_t *t) {
    m_local_targets.emplace_back(t);
    m_targets.emplace(t->id(), t);
}

} // namespace indecorous
