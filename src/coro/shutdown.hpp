#ifndef CORO_SHUTDOWN_HPP_
#define CORO_SHUTDOWN_HPP_

#include <atomic>
#include <cstdint>
#include <list>

#include "sync/file_wait.hpp"

namespace indecorous {

class thread_t;

class shutdown_t {
public:
    shutdown_t();
    void shutdown(std::list<thread_t> *threads);
    bool update(int64_t active_delta);
    void reset(uint64_t initial_count);

    file_callback_t *get_file_event();
private:
    // To be triggered once activity has ceased - each other thread
    // should then wake up and subsequently halt operations
    class shutdown_file_event_t : public file_callback_t {
    public:
        shutdown_file_event_t();
        void set();
        void reset();
    private:
        void file_callback(wait_result_t result);
        scoped_fd_t m_scoped_fd;
    }

    shutdown_file_event_t m_file_event;
    std::atomic<uint64_t> m_active_count;
};

} // namespace indecorous

#endif // CORO_SHUTDOWN_HPP_
