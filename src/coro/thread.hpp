#ifndef CORO_THREAD_HPP_
#define CORO_THREAD_HPP_

#include <cstddef>
#include <cstdint>
#include <map>
#include <thread>

#include "rpc/target.hpp"
#include "sync/file_wait.hpp"

struct pollfd;

namespace indecorous {

class scheduler_t;
class thread_barrier_t;

class thread_t {
public:
    static void add_timer(wait_callback_t* timer, uint32_t timeout);
    static void update_timer(wait_callback_t* timer, uint32_t timeout);
    static void remove_timer(wait_callback_t* timer);

    static bool add_file_wait(int fd, int event_mask, wait_callback_t* waiter);
    static bool remove_file_wait(int fd, int event_mask, wait_callback_t* waiter);

    thread_t(scheduler_t* parent, thread_barrier_t *barrier);

    static thread_t *self();

    dispatcher_t *dispatcher();
    void shutdown();
    target_id_t id() const;

private:
    void main();

    void do_wait();
    void build_poll_set(struct pollfd* poll_array);
    void notify_waiters(size_t size, struct pollfd* poll_array);
    void notify_waiters_for_event(struct pollfd* poll_event, int event_mask);

    int get_wait_timeout();
    static uint64_t get_end_time(uint32_t timeout);

    friend class coro_timer_t;
    friend void coro_pull();

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
    thread_barrier_t *m_barrier;
    dispatcher_t m_dispatch;

    // TODO: make these use intrusive queues or something for performance?
    // TODO: also consider unordered maps
    std::multimap<file_wait_info_t, wait_callback_t*> m_file_waiters;
    std::multimap<uint64_t, wait_callback_t*> m_timer_waiters;

    local_target_t m_target;
    std::thread m_thread;

    static __thread thread_t* s_instance;
};

} // namespace indecorous

#endif // CORO_THREAD_HPP_
