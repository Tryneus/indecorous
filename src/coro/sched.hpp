#ifndef CORO_CORO_SCHED_HPP_
#define CORO_CORO_SCHED_HPP_

#include <unordered_set>

#include "coro/barrier.hpp"
#include "rpc/hub.hpp"
#include "rpc/id.hpp"

namespace indecorous {

class dispatcher_t;
class thread_t;

class scheduler_t {
public:
    explicit scheduler_t(size_t num_threads);
    ~scheduler_t();

    // This function will not return until all coroutines (and children thereof) return
    void run();

    // TODO: get a more portable definition of target ids - these will change from node to node
    // UUIDs?
    const std::unordered_set<target_id_t> &all_threads() const;

    message_hub_t *message_hub();

private:
    static scheduler_t &get_instance();
    static scheduler_t* s_instance;

    bool m_running;
    size_t m_num_threads;
    thread_barrier_t m_barrier;
    message_hub_t m_message_hub; // For passing messages between threads/processes
    std::unordered_set<target_id_t> m_thread_ids;
    thread_t** m_threads;
};

} // namespace indecorous

#endif
