#ifndef RPC_STREAM_HPP_
#define RPC_STREAM_HPP_

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>

#include "containers/buffer.hpp"
#include "containers/file.hpp"
#include "containers/intrusive.hpp"

namespace indecorous {

class wait_object_t;
class write_message_t;
class read_message_t;

class stream_t {
public:
    virtual ~stream_t();
    virtual void write(write_message_t &&) = 0;
    virtual read_message_t read() = 0;
    virtual void wait(wait_object_t *) = 0;
};

class local_stream_t : public stream_t {
public:
    explicit local_stream_t();
    void write(write_message_t &&msg);
    read_message_t read();
    void wait(wait_object_t *interruptor);
private:
    friend class scheduler_t; // To get the initial queue size
    friend class thread_t; // To check empty at the end of a run
    scoped_fd_t fd;
    mpsc_queue_t<linkable_buffer_t> message_queue;
};

class tcp_stream_t : public stream_t {
public:
    explicit tcp_stream_t(int _fd);
    void write(write_message_t &&msg);
    read_message_t read();
    void wait(wait_object_t *interruptor);
private:
    friend class read_message_t;
    void read_exactly(char *buffer, size_t data);
    void write_exactly(char *buffer, size_t data);
    int fd;
};

} // namespace indecorous

#endif // STREAM_HPP_
