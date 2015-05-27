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

local_target_t::local_target_t(message_hub_t *hub, thread_t *thread) :
    target_t(hub), m_stream(thread) { }

bool local_target_t::handle() {
    read_message_t msg(m_stream.read());
    if (!msg.buffer.has()) { return false; }
    membership.hub->spawn(std::move(msg));
    return true;
}

stream_t *local_target_t::stream() {
    return &m_stream;
}

remote_target_t::remote_target_t(message_hub_t *hub) :
    target_t(hub), m_stream(-1) { }

stream_t *remote_target_t::stream() {
    return &m_stream;
}

} // namespace indecorous
