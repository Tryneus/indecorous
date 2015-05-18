#include "coro/thread.hpp"

#include <limits>
#include <sys/time.h>

#include "coro/sched.hpp"
#include "errors.hpp"

namespace indecorous {

__thread thread_t* thread_t::s_instance = nullptr;
__thread size_t thread_t::s_thread_id = -1;

struct thread_args_t {
    thread_t* instance;
    size_t thread_id;
};

thread_t::thread_t(scheduler_t* parent,
                   size_t thread_id,
                   pthread_barrier_t* barrier) :
        m_parent(parent),
        m_shutdown(false),
        m_barrier(barrier),
        m_target(&parent->m_message_hub, this) {
    // Launch pthread for the new thread, then return
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    thread_args_t* args = new thread_args_t;

    args->instance = this;
    args->thread_id = thread_id;

    pthread_create(&m_pthread, &attr, &thread_hook, args);
    pthread_attr_destroy(&attr);
}

void* thread_t::thread_hook(void* param) {
    thread_args_t *args = reinterpret_cast<thread_args_t*>(param);
    s_thread_id = args->thread_id;
    s_instance = args->instance;
    delete args;

    s_instance->m_dispatch = new dispatcher_t();

    s_instance->thread_main();

    delete s_instance->m_dispatch;
    s_instance->m_dispatch = nullptr;

    // Last barrier is for the scheduler_t destructor,
    //  indicates that the thread_t objects are safe for destruction
    pthread_barrier_wait(s_instance->m_barrier);

    return nullptr;
}

void thread_t::thread_main() {
    // First barrier is for the scheduler_t constructor,
    //  indicates that the dispatchers are ready
    pthread_barrier_wait(m_barrier);

    while (true) {
        // Wait for run or ~scheduler_t
        pthread_barrier_wait(m_barrier);

        if (m_shutdown)
            break;

        while (m_dispatch->run() > 0) {
            do_wait();
        }

        // Wait for other threads to finish
        pthread_barrier_wait(m_barrier);
    }
}

void thread_t::shutdown() {
    m_shutdown = true;
}

thread_t& thread_t::get_instance() {
    assert(s_instance != nullptr);
    return *s_instance;
}

void thread_t::build_poll_set(struct pollfd* poll_array) {
    // TODO don't fully generate this every time
    // Build up poll array for subscribed file events
    size_t index = 0;

    poll_array[index].fd = 0;
    poll_array[index].events = 0;
    for (auto i = m_file_waiters.begin(); i != m_file_waiters.end(); ++i) {
        if (poll_array[index].fd == 0) {
            poll_array[index].fd = i->first.fd;
            poll_array[index].events = i->first.event;
        } else if (i->first.fd == poll_array[index].fd) {
            poll_array[index].events |= i->first.event;
        } else {
            ++index;
            poll_array[index].fd = 0;
            poll_array[index].events = 0;
        }
    }
}

// TODO: pass error events using WaitError or whatever
void thread_t::notify_waiters(size_t size, struct pollfd* poll_array) {
    // Call any triggered file waiters
    for (size_t index = 0; index < size; ++index) {
        // Check each flag and call associated file waiters
        notify_waiters_for_event(&poll_array[index], POLLIN);
        notify_waiters_for_event(&poll_array[index], POLLOUT);
        notify_waiters_for_event(&poll_array[index], POLLERR);
        notify_waiters_for_event(&poll_array[index], POLLHUP);
        notify_waiters_for_event(&poll_array[index], POLLPRI);
        notify_waiters_for_event(&poll_array[index], POLLRDHUP);
    }
}

void thread_t::notify_waiters_for_event(struct pollfd* pollEvent, int event_mask) {
    if (pollEvent->revents & event_mask) {
        file_wait_info_t info = { pollEvent->fd, event_mask };

        for (auto i = m_file_waiters.lower_bound(info);
             i != m_file_waiters.end() && i->first == info; ++i)
            i->second->wait_callback(wait_result_t::Success);
    }
}

void thread_t::do_wait() {
    // Wait on timers and file descriptors
    size_t pollSize = m_file_waiters.size(); // Check if this works for multimaps
    struct pollfd poll_array[1]; // TODO: RSI: fix variable length array

    build_poll_set(poll_array);
    int timeout = get_wait_timeout(); // This will handle any timer callbacks

    // Do the wait
    int res;
    do {
        res = poll(poll_array, pollSize, timeout);
    } while (res == -1 && errno == EINTR);

    if (res == -1)
        throw coro_exc_t("poll failed");
    else if (res != 0)
        notify_waiters(pollSize, poll_array);
    else
        get_wait_timeout(); // Time out, a timer has probably expired, so notify the waiter
}

int thread_t::get_wait_timeout() {
    if (m_dispatch->m_runQueue.size() > 0)
        return 0;

    if (m_timer_waiters.empty())
        return -1;

    uint64_t now = get_end_time(0);
    uint64_t delta = std::numeric_limits<int>::max();

    // Notify any elapsed timers and find the delta until the following timer
    for (auto i = m_timer_waiters.begin(); !m_timer_waiters.empty(); i = m_timer_waiters.begin()) {
        if (i->first <= now) {
            i->second->wait_callback(wait_result_t::Success);
            m_timer_waiters.erase(i);
        } else {
            delta = i->first - now;
            break;
        }
    }

    return (delta > std::numeric_limits<int>::max()) ?
        std::numeric_limits<int>::max() : static_cast<int>(delta);
}

uint64_t thread_t::get_end_time(uint32_t timeout) {
    struct timeval time;
    if (gettimeofday(&time, nullptr) != 0)
        throw coro_exc_t("gettimeofday failed");

    uint64_t now = (time.tv_usec / 1000) + (time.tv_sec * 1000);
    return (now + timeout);
}

void thread_t::add_timer(wait_callback_t* cb, uint32_t timeout) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();

    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i == thread.m_timer_waiters.end());

    uint64_t endTime = get_end_time(timeout);
    thread.m_timer_waiters.insert(std::make_pair(endTime, cb));
}

void thread_t::update_timer(wait_callback_t* cb, uint32_t timeout) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();

    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i != thread.m_timer_waiters.end());
    thread.m_timer_waiters.erase(i);

    uint64_t endTime = get_end_time(timeout);
    thread.m_timer_waiters.insert(std::make_pair(endTime, cb));
}

void thread_t::remove_timer(wait_callback_t* cb) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();
    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i != thread.m_timer_waiters.end());
    thread.m_timer_waiters.erase(i);
}

bool thread_t::add_file_wait(int fd, int event_mask, wait_callback_t* cb) {
    assert(event_mask == POLLRDHUP ||
           event_mask == POLLERR ||
           event_mask == POLLHUP ||
           event_mask == POLLOUT ||
           event_mask == POLLPRI ||
           event_mask == POLLIN);

    thread_t& thread = get_instance();
    file_wait_info_t info = { fd, event_mask };

    for (auto i = thread.m_file_waiters.lower_bound(info);
         i != thread.m_file_waiters.end() && i->first == info; ++i) {
        if (i->second == cb)
            return false;
    }

    thread.m_file_waiters.insert(std::make_pair(info, cb));
    return true;
}

bool thread_t::remove_file_wait(int fd, int event_mask, wait_callback_t* cb) {
    thread_t& thread = get_instance();
    file_wait_info_t info = { fd, event_mask };

    // Find the specified item in the map
    for (auto i = thread.m_file_waiters.lower_bound(info);
         i != thread.m_file_waiters.end() && i->first == info; ++i) {
        if (i->second == cb) {
            thread.m_file_waiters.erase(i);
            return true;
        }
    }

    return false;
}

} // namespace indecorous

