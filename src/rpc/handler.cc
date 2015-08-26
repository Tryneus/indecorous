#include "rpc/handler.hpp"

#include "rpc/hub.hpp"

namespace indecorous {

std::unordered_map<rpc_id_t, rpc_callback_t *> &register_callback(rpc_callback_t *cb) {
    static std::unordered_map<rpc_id_t, rpc_callback_t *> rpcs;
    if (cb != nullptr) {
        GUARANTEE(rpcs.emplace(cb->id(), cb).second);
    }
    return rpcs;
}

} // namespace indecorous

