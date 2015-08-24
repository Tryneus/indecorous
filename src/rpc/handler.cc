#include "rpc/handler.hpp"

#include "rpc/hub.hpp"

namespace indecorous {

std::unordered_map<rpc_id_t, handler_callback_t *> &register_handler(handler_callback_t *cb) {
    static std::unordered_map<rpc_id_t, handler_callback_t *> handlers;
    if (cb != nullptr) {
        GUARANTEE(handlers.emplace(cb->id(), cb).second);
    }
    return handlers;
}

} // namespace indecorous

