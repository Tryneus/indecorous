#ifndef RPC_HUB_HPP_
#define RPC_HUB_HPP_

#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "coro/coro.hpp"
#include "sync/multiple_wait.hpp"
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
    message_hub_t(target_t *self_target, target_t *io_target);
    ~message_hub_t();

    target_t *target(target_id_t id);
    target_t *io_target();

    const std::vector<target_t *> &local_targets();

    template <typename RPC, typename... Args>
    size_t broadcast_local_noreply(Args &&...args) {
        for (auto &&t : m_local_targets) {
            t->call_noreply<RPC>(std::forward<Args>(args)...);
        }
        return m_local_targets.size();
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr()))::result_t>
    std::vector<future_t<Res> > broadcast_local_async(Args &&...args) {
        std::vector<future_t<Res> > futures;
        futures.reserve(m_local_targets.size());
        for (auto &&t : m_local_targets) {
            futures.emplace_back(t->call_async<RPC>(std::forward<Args>(args)...));
        }
        return futures;
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr()))::result_t>
    typename std::enable_if<std::is_void<Res>::value, void>::type
    broadcast_local_sync(Args &&...args) {
        std::vector<future_t<Res> > futures = broadcast_local_async<RPC>(std::forward<Args>(args)...);
        wait_all(futures);
    }

    template <typename RPC, typename... Args,
              typename Res = typename decltype(rpc_bridge(RPC::fn_ptr()))::result_t>
    typename std::enable_if<!std::is_void<Res>::value, std::vector<Res> >::type
    broadcast_local_sync(Args &&...args) {
        std::vector<future_t<Res> > futures = broadcast_local_async<RPC>(std::forward<Args>(args)...);
        std::vector<Res> res;
        res.reserve(futures.size());
        wait_all(futures);
        for (auto &&f : futures) {
            res.emplace_back(f.release());
        }
        return res;
    }

private:
    friend class scheduler_t; // For initializing local and io targets
    void add_local_target(target_t *t);

    // For spawning received RPCs
    friend class thread_t;
    friend class io_thread_t;
    void spawn_task(read_message_t msg);

    friend class target_t; // For generating new request ids and promises
    target_t::request_params_t new_request();

    void handle(task_id_t task_id, rpc_callback_t *rpc, read_message_t msg);

    const target_id_t m_self_target_id;
    target_t * const m_io_target;
    std::vector<target_t *> m_local_targets;
    std::unordered_map<target_id_t, target_t *> m_targets;
    std::unordered_map<rpc_id_t, rpc_callback_t *> m_rpcs;

    id_generator_t<request_id_t> m_request_gen;
    id_generator_t<task_id_t> m_task_gen;
    std::unordered_map<request_id_t, promise_t<read_message_t> > m_replies;

    DISABLE_COPYING(message_hub_t);
};

} // namespace indecorous

#endif // HUB_HPP_
