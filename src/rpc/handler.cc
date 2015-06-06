#include "rpc/handler.hpp"

#include "rpc/hub.hpp"

namespace indecorous {

handler_callback_t::~handler_callback_t() { }

void send_reply(message_hub_t *hub, target_id_t source_id, write_message_t msg) {
    hub->send_reply(source_id, std::move(msg));
}

} // namespace indecorous

