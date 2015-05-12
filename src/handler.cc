#include "handler.hpp"

#include "hub.hpp"

message_callback_t::message_callback_t(message_hub_t *hub) :
        parent_hub(hub) {
    parent_hub->add_handler(this);
}

message_callback_t::~message_callback_t() {
    parent_hub->remove_handler(this);
}
