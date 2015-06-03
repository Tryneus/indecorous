#ifndef RPC_HUB_HPP_
#define RPC_HUB_HPP_

#include <string>
#include <unordered_map>

#include "rpc/id.hpp"

namespace indecorous {

class target_t;
class local_target_t;
class handler_callback_t;
class read_message_t;
class write_message_t;

class message_hub_t {
public:
    message_hub_t();
    ~message_hub_t();

    local_target_t *self_target();
    target_t *target(target_id_t id);

    void send_reply(target_id_t target_id, write_message_t &&msg);

private:
    bool spawn(read_message_t msg);

    std::unordered_map<target_id_t, target_t *> targets;
    std::unordered_map<handler_id_t, handler_callback_t *> handlers;
};

} // namespace indecorous

#endif // HUB_HPP_
