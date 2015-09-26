#ifndef RPC_STREAM_HPP_
#define RPC_STREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <queue>

#include "containers/buffer.hpp"
#include "containers/file.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class waitable_t;
class write_message_t;
class read_message_t;

class stream_t {
public:
    virtual ~stream_t();
    virtual void write(write_message_t &&) = 0;
    virtual read_message_t read() = 0;
    virtual void wait(waitable_t *) = 0;
};

class local_stream_t final : public stream_t {
public:
    local_stream_t();
    void write(write_message_t &&msg) override final;
    read_message_t read() override final;
    void wait(waitable_t *interruptor) override final;
private:
    friend class scheduler_t; // To get the initial queue size
    friend class thread_t; // To check empty at the end of a run
    scoped_fd_t m_fd;
    mpsc_queue_t<linkable_buffer_t> m_message_queue;
};

class tcp_stream_t final : public stream_t {
public:
    explicit tcp_stream_t(int _fd);
    void write(write_message_t &&msg) override final;
    read_message_t read() override final;
    void wait(waitable_t *interruptor) override final;
private:
    friend class read_message_t;
    void read_exactly(char *buffer, size_t data);
    void write_exactly(char *buffer, size_t data);
    int m_fd;
};

} // namespace indecorous

#endif // STREAM_HPP_
