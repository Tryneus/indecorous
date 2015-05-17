#ifndef RPC_HUB_HPP_
#define RPC_HUB_HPP_

#include <unordered_map>

#include "rpc/id.hpp"

class target_t;
class handler_callback_t;

class message_hub_t {
public:
    message_hub_t();
    ~message_hub_t();

    target_t *get_target(target_id_t id) const;

    template <class T>
    class membership_t {
    public:
        membership_t(message_hub_t *_hub, T *_member);
        ~membership_t();
    private:
        message_hub_t *hub;
        T *member;
    };

private:
    void add(target_t *target);
    void remove(target_t *target);
    void add(handler_callback_t *callback);
    void remove(handler_callback_t *callback);

    std::unordered_map<target_id_t,
                       target_t *,
                       target_id_t::hash_t,
                       target_id_t::equal_t>
        targets;

    std::unordered_map<handler_id_t,
                       handler_callback_t *,
                       handler_id_t::hash_t,
                       handler_id_t::equal_t>
        callbacks;
};

#endif // HUB_HPP_
