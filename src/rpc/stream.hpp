#ifndef RPC_STREAM_HPP_
#define RPC_STREAM_HPP_

#include <semaphore.h>

#include <cstddef>
#include <cstdint>
#include <queue>

#include "containers/buffer.hpp"
#include "containers/file.hpp"
#include "containers/intrusive.hpp"
#include "cross_thread/spinlock.hpp"

namespace indecorous {

class write_message_t;
class read_message_t;

class stream_t {
public:
    virtual ~stream_t();
    virtual void write(write_message_t &&) = 0;
    virtual read_message_t read() = 0;
    virtual void wait() = 0;
};

class local_stream_t final : public stream_t {
public:
    local_stream_t();
    void write(write_message_t &&msg) override final;
    read_message_t read() override final;

    // `wait()` must only be called from within a coroutine context
    void wait() override final;

    // `size()` is not thread-safe, only use it when threads are not running
    size_t size() const;

private:
    scoped_fd_t m_fd;
    mpsc_queue_t<linkable_buffer_t> m_queue;
};

class io_stream_t final : public stream_t {
public:
    io_stream_t();
    void write(write_message_t &&msg) override final;

    void wait() override final;

    // `read()` must only be called from within a coroutine context
    read_message_t read() override final;

    // `size()` is not thread-safe, only use it when threads are not running
    size_t size() const;

private:
    scoped_fd_t m_fd;
    spinlock_t m_read_lock;
    mpsc_queue_t<linkable_buffer_t> m_queue;
};

class tcp_stream_t final : public stream_t {
public:
    explicit tcp_stream_t(int _fd);
    void write(write_message_t &&msg) override final;
    read_message_t read() override final;
    void wait() override final;
private:
    friend class read_message_t;
    void read_exactly(char *buffer, size_t data);
    void write_exactly(char *buffer, size_t data);
    int m_fd;
};

} // namespace indecorous

#endif // STREAM_HPP_
