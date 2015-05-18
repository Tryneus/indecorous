#include "rpc/target.hpp"

#include "rpc/hub.hpp"

namespace indecorous {

target_t::target_t(message_hub_t *hub) :
    target_id(target_id_t::assign()),
    membership(hub, this) { }

target_t::~target_t() { }

target_id_t target_t::id() const {
    return target_id;
}

/*
target_t::sync_request_t::sync_request_t(target_t *_parent,
                                                request_id_t request_id) :
        parent(_parent), id(request_id) {
    parent->waiters.insert(std::make_pair(id, &done_cond));
}
target_t::sync_request_t::~sync_request_t() {
    parent->waiters.erase(id);
}
message_t target_t::sync_request_t::run(message_t &&msg) {
    parent->stream.write(std::move(msg));
    done_cond.wait();
}
*/

local_target_t::local_target_t(message_hub_t *hub, thread_t *thread) :
    target_t(hub), stream_(thread) { }

stream_t *local_target_t::stream() {
    return &stream_;
}

/*
remote_target_t::remote_target_t(message_hub_t *hub) :
    target_t(hub), stream(-1) { }

stream_t *remote_target_t::stream() {
    return &stream_;
}
*/

} // namespace indecorous
