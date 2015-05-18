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

class scheduler_t {
public:
    scheduler_t(size_t num_threads);
    ~scheduler_t();

    // This function will not return until all coroutines (and children thereof) return
    void run();

    // This function will return the target id for the current thread (which can
    // be used to send/receive RPCs.
    target_id_t current_thread();
    const std::set<target_id_t> &all_threads();

    message_hub_t *message_hub();

    class thread_t {
    public:
        static void add_timer(wait_callback_t* timer, uint32_t timeout);
        static void update_timer(wait_callback_t* timer, uint32_t timeout);
        static void remove_timer(wait_callback_t* timer);

        static bool add_file_wait(int fd, int event_mask, wait_callback_t* waiter);
        static bool remove_file_wait(int fd, int event_mask, wait_callback_t* waiter);

        thread_t(scheduler_t* parent, size_t thread_id, pthread_barrier_t *barrier);

    private:
        friend class scheduler_t;

        void shutdown();
        static thread_t& get_instance();

        static void* thread_hook(void* param);
        void thread_main();

        void do_wait();
        void build_poll_set(struct pollfd* poll_array);
        void notify_waiters(size_t size, struct pollfd* poll_array);
        void notify_waiters_for_event(struct pollfd* poll_event, int event_mask);

        int get_wait_timeout();
        static uint64_t get_end_time(uint32_t timeout);

        friend class coro_timer_t;

        struct file_wait_info_t {
            int fd;
            int event;

            // TODO: This is probably the wrong way to do whatever this is supposed to do
            bool operator == (const file_wait_info_t& other) const {
                return (fd == other.fd) && (event == other.event);
            }

            bool operator < (const file_wait_info_t& other) const {
                return (fd < other.fd) || (event < other.event);
            }
        };

        scheduler_t* m_parent;
        bool m_shutdown;
        pthread_t m_pthread;
        pthread_barrier_t *m_barrier;
        dispatcher_t* m_dispatch;

        // TODO: make these use intrusive queues or something for performance?
        // TODO: also consider unordered maps
        std::multimap<file_wait_info_t, wait_callback_t*> m_file_waiters;
        std::multimap<uint64_t, wait_callback_t*> m_timer_waiters;

        local_target_t m_target;

        static __thread thread_t* s_instance;
        static __thread size_t s_thread_id;
    };

private:
    friend class dispatcher_t;
    friend class coro_t;

    static dispatcher_t& get_dispatcher(size_t thread_id);

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
