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
class read_message_t;
class write_message_t;

class message_hub_t {
public:
    message_hub_t();
    ~message_hub_t();

    local_target_t *self_target();
    target_t *target(target_id_t id);

    template <typename RPC, typename... Args>
    size_t broadcast_local_noreply(Args &&...args) {
        for (auto &&t : m_local_targets) {
            t->call_noreply<RPC>(std::forward<Args>(args)...);
        }
        return m_local_targets.size();
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr))::result_t>
    std::vector<future_t<Res> > broadcast_local_async(Args &&...args) {
        std::vector<future_t<Res> > futures;
        futures.reserve(m_local_targets.size());
        for (auto &&t : m_local_targets) {
            futures.emplace_back(t->call_async<RPC>(std::forward<Args>(args)...));
        }
        return futures;
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr))::result_t>
    typename std::enable_if<std::is_void<Res>::value, void>::type broadcast_local_sync(Args &&...args) {
        std::vector<future_t<Res> > futures = broadcast_local_async<RPC>(std::forward<Args>(args)...);
        wait_all_it(futures.begin(), futures.end());
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr))::result_t>
    typename std::enable_if<!std::is_void<Res>::value, std::vector<Res> >::type broadcast_local_sync(Args &&...args) {
        std::vector<future_t<Res> > futures = broadcast_local_async<RPC>(std::forward<Args>(args)...);
        std::vector<Res> res;
        res.reserve(futures.size());
        wait_all_it(futures);
        for (auto &&f : futures) {
            res.emplace_back(f.release());
        }
        return res;
    }

private:
    friend class scheduler_t; // For initializing local targets
    void set_local_targets(std::list<thread_t> *threads);

    friend class local_target_t; // For spawning handlers of messages
    bool spawn(read_message_t msg);

    friend class target_t; // For getting request responses
    future_t<read_message_t> get_response(request_id_t id);

    local_target_t m_self_target;
    std::vector<local_target_t *> m_local_targets;
    std::unordered_map<target_id_t, target_t *> m_targets;
    std::unordered_map<rpc_id_t, rpc_callback_t *> m_rpcs;
    std::unordered_map<request_id_t, promise_t<read_message_t> > m_replies;
};

} // namespace indecorous

#endif // HUB_HPP_
