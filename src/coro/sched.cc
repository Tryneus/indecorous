#include "coro/sched.hpp"

#include <sys/time.h>
#include <limits.h>

#include "coro/coro.hpp"
#include "coro/errors.hpp"

namespace indecorous {

scheduler_t* scheduler_t::s_instance = nullptr;
__thread scheduler_t::thread_t* scheduler_t::thread_t::s_instance = nullptr;
__thread size_t scheduler_t::thread_t::s_thread_id = -1;

scheduler_t::scheduler_t(size_t num_threads) :
        m_running(false),
        m_num_threads(num_threads) {
    if (s_instance != nullptr)
        throw coro_exc_t("scheduler_t instance already exists");

    pthread_barrier_init(&m_barrier, nullptr, m_num_threads + 1);

    // Create thread_t objects for each thread
    m_threads = new thread_t*[m_num_threads];
    for (size_t i = 0; i < m_num_threads; ++i) {
        m_threads[i] = new thread_t(this, i, &m_barrier);
        m_thread_ids.insert(m_threads[i]->m_target.id());
    }

    // Wait for all threads to start up
    pthread_barrier_wait(&m_barrier);

    s_instance = this;
}

scheduler_t::~scheduler_t() {
    assert(s_instance == this);
    assert(!m_running);

    // Tell all threads to shutdown once we hit the following barrier
    for (size_t i = 0; i < m_num_threads; ++i) {
        assert(m_threads[i]->m_dispatch->m_active_contexts == 0);
        m_threads[i]->shutdown();
    }

    // Tell threads to exit
    pthread_barrier_wait(&m_barrier);

    // Wait for threads to exit
    pthread_barrier_wait(&m_barrier);
    pthread_barrier_destroy(&m_barrier);

    for (size_t i = 0; i < m_num_threads; ++i) {
        delete m_threads[i];
    }
    delete [] m_threads;

    s_instance = nullptr;
}

target_id_t scheduler_t::current_thread() {
    return thread_t::get_instance().m_target.id();
}

const std::set<target_id_t> &scheduler_t::all_threads() {
    return m_thread_ids;
}

message_hub_t *scheduler_t::message_hub() {
    return &m_message_hub;
}

dispatcher_t& scheduler_t::get_dispatcher(size_t thread_id) {
    assert(s_instance != nullptr);
    if (thread_id >= s_instance->m_num_threads)
        throw coro_exc_t("thread_id out of bounds");

    return *s_instance->m_threads[thread_id]->m_dispatch;
}

void scheduler_t::run() {
    assert(!m_running);

    // Wait for all threads to get to their loop
    m_running = true;
    pthread_barrier_wait(&m_barrier);

    // Wait for all threads to finish all their coroutines
    pthread_barrier_wait(&m_barrier);
    m_running = false;
}

struct thread_args_t {
    scheduler_t::thread_t* instance;
    size_t thread_id;
};

scheduler_t::thread_t::thread_t(scheduler_t* parent,
                                size_t thread_id,
                                pthread_barrier_t* barrier) :
        m_parent(parent),
        m_shutdown(false),
        m_barrier(barrier),
        m_target(&parent->m_message_hub) {
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

void* scheduler_t::thread_t::thread_hook(void* param) {
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

void scheduler_t::thread_t::thread_main() {
    // First barrier is for the scheduler_t constructor,
    //  indicates that the dispatchers are ready
    pthread_barrier_wait(m_barrier);

    while (true) {
        // Wait for scheduler_t::run or ~scheduler_t
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

void scheduler_t::thread_t::shutdown() {
    m_shutdown = true;
}

scheduler_t::thread_t& scheduler_t::thread_t::get_instance() {
    assert(s_instance != nullptr);
    return *s_instance;
}

void scheduler_t::thread_t::build_poll_set(struct pollfd* poll_array) {
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
void scheduler_t::thread_t::notify_waiters(size_t size, struct pollfd* poll_array) {
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

void scheduler_t::thread_t::notify_waiters_for_event(struct pollfd* pollEvent, int event_mask) {
    if (pollEvent->revents & event_mask) {
        file_wait_info_t info = { pollEvent->fd, event_mask };

        for (auto i = m_file_waiters.lower_bound(info);
             i != m_file_waiters.end() && i->first == info; ++i)
            i->second->wait_callback(wait_result_t::Success);
    }
}

void scheduler_t::thread_t::do_wait() {
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

int scheduler_t::thread_t::get_wait_timeout() {
    if (m_dispatch->m_runQueue.size() > 0)
        return 0;

    if (m_timer_waiters.empty())
        return -1;

    uint64_t now = get_end_time(0);
    uint64_t delta = INT_MAX;

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

    return (delta > INT_MAX) ? INT_MAX : (int)delta;
}

uint64_t scheduler_t::thread_t::get_end_time(uint32_t timeout) {
    struct timeval time;
    if (gettimeofday(&time, nullptr) != 0)
        throw coro_exc_t("gettimeofday failed");

    uint64_t now = (time.tv_usec / 1000) + (time.tv_sec * 1000);
    return (now + timeout);
}

void scheduler_t::thread_t::add_timer(wait_callback_t* cb, uint32_t timeout) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();

    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i == thread.m_timer_waiters.end());

    uint64_t endTime = get_end_time(timeout);
    thread.m_timer_waiters.insert(std::make_pair(endTime, cb));
}

void scheduler_t::thread_t::update_timer(wait_callback_t* cb, uint32_t timeout) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();

    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i != thread.m_timer_waiters.end());
    thread.m_timer_waiters.erase(i);

    uint64_t endTime = get_end_time(timeout);
    thread.m_timer_waiters.insert(std::make_pair(endTime, cb));
}

void scheduler_t::thread_t::remove_timer(wait_callback_t* cb) {
    // TODO: do a better than O(n) search - store expiration time in timer?
    thread_t& thread = get_instance();
    auto i = thread.m_timer_waiters.begin();
    for (; i != thread.m_timer_waiters.end() && i->second != cb; ++i);
    assert(i != thread.m_timer_waiters.end());
    thread.m_timer_waiters.erase(i);
}

bool scheduler_t::thread_t::add_file_wait(int fd, int event_mask, wait_callback_t* cb) {
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

bool scheduler_t::thread_t::remove_file_wait(int fd, int event_mask, wait_callback_t* cb) {
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

