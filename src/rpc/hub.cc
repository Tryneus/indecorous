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
    m_task_gen(),
    m_replies() { }

message_hub_t::~message_hub_t() { }

void message_hub_t::handle(task_id_t task_id, rpc_callback_t *rpc, read_message_t msg) {
    target_id_t source_id = msg.source_id;
    logDebug("Starting task %" PRIu64, task_id.value());

    if (msg.request_id == request_id_t::noreply()) {
        rpc->handle_noreply(std::move(msg));
    } else {
        write_message_t reply = rpc->handle(std::move(msg));

        target_t *source = target(source_id);
        if (source != nullptr) {
            source->send_reply(std::move(reply));
        } else {
            logInfo("Could not find target (%" PRIu64 ") for reply, might be disconnected", source_id.value());
        }
    }

    logDebug("Finished task %" PRIu64, task_id.value());
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
            logInfo("Orphan reply encountered for request %" PRIu64 ":%" PRIu64,
                    msg.source_id.value(), msg.request_id.value());
        }
    } else {
        if (msg.source_id.is_local()) {
            thread_t::self()->dispatcher()->note_accepted_task();
        }

        auto cb_it = m_rpcs.find(msg.rpc_id);
        if (msg.request_id == request_id_t::noreply() || cb_it != m_rpcs.end()) {
            const task_id_t task_id = m_task_gen.next();
            coro_t::spawn_detached(&message_hub_t::handle, this, task_id, cb_it->second, std::move(msg));
        } else {
            logError("No registered RPC for rpc_id (%lu).", msg.rpc_id.value());
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
