#ifndef CORO_CORO_SCHED_HPP_
#define CORO_CORO_SCHED_HPP_
#include <poll.h>
#include <stdint.h>
#include <atomic>
#include <map>
#include <set>
#include <pthread.h>

#include "sync/wait_object.hpp"
#include "rpc/target.hpp"
#include "rpc/hub.hpp"

namespace indecorous {

class dispatcher_t;
class thread_t;

class scheduler_t {
public:
    scheduler_t(size_t num_threads);
    ~scheduler_t();

    // This function will not return until all coroutines (and children thereof) return
    void run();

    // TODO: get a more portable definition of target ids - these will change from node to node
    // UUIDs?
    const std::set<target_id_t> &all_threads() const;

    message_hub_t *message_hub();

private:
    friend class dispatcher_t;
    friend class thread_t;
    friend class coro_t;

    static scheduler_t &get_instance();

    bool m_running;
    pthread_barrier_t m_barrier;
    thread_t** m_threads;
    size_t m_num_threads;
    static scheduler_t* s_instance;
    message_hub_t m_message_hub; // For passing messages between threads/processes
    std::set<target_id_t> m_thread_ids;
};

} // namespace indecorous

#endif
