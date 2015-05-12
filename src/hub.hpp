#ifndef HUB_HPP_
#define HUB_HPP_

#include <unordered_map>

#include "id.hpp"
#include "handler.hpp"

class target_t;
class message_callback_t;

class message_hub_t {
public:
    message_hub_t();
    ~message_hub_t();

    target_t *get_target(target_id_t id) const;
    message_callback_t *get_handler(handler_id_t id) const;

private:
    friend class message_callback_t;
    void add_handler(message_callback_t *callback);
    void remove_handler(message_callback_t *callback);

    friend class target_t;
    void add_target(target_t *target);
    void remove_target(target_t *target);

    std::unordered_map<target_id_t,
                       target_t *,
                       target_id_t::hash_t,
                       target_id_t::equal_t>
        targets;

    std::unordered_map<handler_id_t,
                       message_callback_t *,
                       handler_id_t::hash_t,
                       handler_id_t::equal_t>
        callbacks;
};

#endif // HUB_HPP_
