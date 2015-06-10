#ifndef RPC_HUB_HPP_
#define RPC_HUB_HPP_

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "rpc/handler.hpp"
#include "rpc/id.hpp"
#include "rpc/target.hpp"

namespace indecorous {

class target_t;
class thread_t;
class handler_callback_t;
class read_message_t;
class write_message_t;

class message_hub_t {
public:
    message_hub_t();
    ~message_hub_t();

    local_target_t *self_target();
    target_t *target(target_id_t id);

    template <typename Callback, typename... Args>
    void broadcast_local_noreply(Args &&...args) {
        for (auto &&t : m_local_targets) {
            t->call_noreply<Callback>(std::forward<Args>(args)...);
        }
    }

    template <typename Callback, typename Res = typename handler_wrapper_t<Callback>::result_t, typename... Args>
    std::vector<future_t<Res> > broadcast_local_async(Args &&...args) {
        std::vector<future_t<Res> > futures;
        for (auto &&t : m_local_targets) {
            futures.emplace_back(t->call_async(std::forward<Args>(args)...));
        }
        return futures;
    }

    template <typename Callback, typename Res = typename handler_wrapper_t<Callback>::result_t, typename... Args>
    std::vector<Res> broadcast_local_sync(Args &&...args) {
        std::vector<future_t<Res> > futures = broadcast_local_async(std::forward<Args>(args)...);
        std::vector<Res> res;
        res.reserve(futures.size());
        wait_all(futures.begin(), futures.end());
        for (auto &&f : futures) {
            res.emplace_back(f.release());
        }
        return res;
    }

    void send_reply(target_id_t target_id, write_message_t &&msg);

private:
    friend class scheduler_t; // For initializing local targets
    void set_local_targets(std::list<thread_t> *threads);

    friend class local_target_t; // For spawning handlers of messages
    bool spawn(read_message_t msg);

    local_target_t m_self_target;
    std::vector<local_target_t *> m_local_targets;
    std::unordered_map<target_id_t, target_t *> m_targets;
    std::unordered_map<handler_id_t, handler_callback_t *> m_handlers;
};

} // namespace indecorous

#endif // HUB_HPP_
